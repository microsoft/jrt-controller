//go:build !ci
// +build !ci

package cache

import (
	"context"
	"errors"
	"testing"

	"github.com/sirupsen/logrus"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

const (
	key1 = "1"
	key2 = "2"
)

var (
	// docker run --rm -it -p 6379:6379 mcr.microsoft.com/cbl-mariner/base/redis:6 redis-server --protected-mode no
	opts   = Options{enabled: true, ip: "localhost", port: 6379}
	value1 = TestStruct{Field1: "value1", Field2: 1}
	value2 = TestStruct{Field1: "value2", Field2: 2}
)

type TestStruct struct {
	Field1 string
	Field2 int
}

func seed() (*Store[string, TestStruct], error) {
	client, err := NewClient(context.TODO(), logrus.New(), &opts)
	if err != nil {
		return nil, err
	}
	s, err := NewStore[string, TestStruct](client, func(key string) (string, error) { return "TestStruct/" + key, nil })
	if err != nil {
		return nil, err
	}

	return s, errors.Join(
		s.Set(key1, &value1),
		s.Set(key2, &value2),
	)
}

func cleanup(store *Store[string, TestStruct]) error {
	return store.client.inner.FlushDB(store.client.ctx).Err()
}

func TestRead(t *testing.T) {
	store, err := seed()
	require.Nil(t, err)
	defer func() {
		if err := cleanup(store); err != nil {
			t.Log(err)
		}
	}()

	out, exists, err := store.Get(key1)
	require.Nil(t, err)
	require.True(t, exists)
	assert.Equal(t, value1.Field1, out.Field1)
	assert.Equal(t, value1.Field2, out.Field2)

	out2, exists2, err2 := store.Get(key2)
	require.Nil(t, err2)
	require.True(t, exists2)
	assert.Equal(t, value2.Field1, out2.Field1)
	assert.Equal(t, value2.Field2, out2.Field2)
}

func TestReadMissingKey(t *testing.T) {
	store, err := seed()
	require.Nil(t, err)
	defer func() {
		if err := cleanup(store); err != nil {
			t.Log(err)
		}
	}()

	out, exists, err := store.Get("missing")
	require.Nil(t, out)
	require.Nil(t, err)
	require.False(t, exists)
}
