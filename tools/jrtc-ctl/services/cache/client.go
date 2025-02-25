package cache

import (
	"context"
	"fmt"

	"github.com/redis/go-redis/v9"
	"github.com/sirupsen/logrus"
)

// Client is a caching client
type Client struct {
	ctx    context.Context
	inner  *redis.Client
	logger *logrus.Logger
	opts   *Options
}

// NewClient returns a new redis client
func NewClient(ctx context.Context, logger *logrus.Logger, o *Options) (*Client, error) {
	var inner *redis.Client
	if o.enabled {
		inner = redis.NewClient(&redis.Options{
			Addr:     fmt.Sprintf("%s:%d", o.ip, o.port),
			Password: "",
			DB:       0,
		})

		if err := inner.Ping(ctx).Err(); err != nil {
			return nil, err
		}
	}

	return &Client{ctx: ctx, logger: logger, inner: inner, opts: o}, nil
}
