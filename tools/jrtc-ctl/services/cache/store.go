// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
package cache

import (
	"encoding/json"
	"fmt"
	"sync"

	"github.com/redis/go-redis/v9"
	"github.com/sirupsen/logrus"
)

// Store is store for some data type
type Store[TKey any, T any] struct {
	client         *Client
	inMemStore     map[string]*T
	keyTransformer func(TKey) (string, error)
	l              *logrus.Entry
	mu             sync.RWMutex
}

// NewStore creates a new store
func NewStore[TKey, T any](c *Client, keyTransformer func(TKey) (string, error)) (*Store[TKey, T], error) {
	var item T
	l := c.logger.WithField("type", fmt.Sprintf("%T", item))
	return &Store[TKey, T]{
		client:         c,
		inMemStore:     make(map[string]*T),
		keyTransformer: keyTransformer,
		l:              l,
	}, nil
}

// Get gets a value from the store
func (s *Store[TKey, T]) Get(key TKey) (*T, bool, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	queryKey, err := s.keyTransformer(key)
	if err != nil {
		return nil, false, err
	}

	l := s.l.WithFields(logrus.Fields{"key": queryKey, "cmd": "get"})

	current, ok := s.inMemStore[queryKey]
	if ok {
		l.Trace("found in memory")
		return current, true, nil
	}

	if !s.client.opts.enabled {
		l.Trace("skipping redis lookup as disabled")
		return nil, false, nil
	}

	out, err := s.client.inner.Get(s.client.ctx, queryKey).Result()
	if err != nil && err != redis.Nil {
		return nil, false, err
	} else if err == redis.Nil {
		return nil, false, nil
	}
	l.Trace("found in redis")
	var ret T
	if err = json.Unmarshal([]byte(out), &ret); err != nil {
		return nil, false, err
	}
	s.inMemStore[queryKey] = &ret
	l.Trace("deserialized from redis")
	return &ret, true, nil
}

// Set sets a value in the store
func (s *Store[TKey, T]) Set(key TKey, value *T) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	queryKey, err := s.keyTransformer(key)
	if err != nil {
		return err
	}

	s.inMemStore[queryKey] = value

	if !s.client.opts.enabled {
		return nil
	}

	b, err := json.Marshal(value)
	if err != nil {
		return err
	}
	return s.client.inner.Set(s.client.ctx, queryKey, b, 0).Err()
}

// Delete deletes a key from the store
func (s *Store[TKey, T]) Delete(key TKey) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	queryKey, err := s.keyTransformer(key)
	if err != nil {
		return err
	}

	delete(s.inMemStore, queryKey)
	if !s.client.opts.enabled {
		return nil
	}
	return s.client.inner.Del(s.client.ctx, queryKey).Err()
}
