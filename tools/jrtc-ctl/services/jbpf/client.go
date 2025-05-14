// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jbpf

import (
	"bytes"
	"context"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/sirupsen/logrus"
)

// Client is a jbpf client
type Client struct {
	ctx    context.Context
	inner  *http.Client
	logger *logrus.Logger
	opts   *Options
}

// NewClient creates a new jbpf client
func NewClient(ctx context.Context, logger *logrus.Logger, opts *Options) (*Client, error) {
	return &Client{
		ctx:    ctx,
		logger: logger,
		inner:  &http.Client{},
		opts:   opts,
	}, nil
}

func (c *Client) doPost(relativeURL string, body any) error {
	childCtx, cancel := context.WithCancel(c.ctx)
	defer cancel()

	bodyB, err := json.Marshal(body)
	if err != nil {
		return err
	}

	req, err := http.NewRequestWithContext(childCtx, "POST", fmt.Sprintf("%s%s", c.opts.addr.String(), relativeURL), bytes.NewBuffer(bodyB))
	if err != nil {
		return err
	}
	req.Header.Set("Content-Type", "application/json")

	hash := sha256.Sum256([]byte(fmt.Sprintf("%s%s%s", c.opts.addr.String(), relativeURL, bodyB)))
	reqHash := hex.EncodeToString(hash[:])

	l := c.logger.WithFields(logrus.Fields{"method": req.Method, "url": req.URL.String()})
	l.WithField("body", string(bodyB)).WithField("request_id", reqHash).Trace("sending http request")
	resp, err := c.inner.Do(req)
	if err != nil {
		return err
	}
	l.WithField("statusCode", resp.StatusCode).WithField("request_id", reqHash).Trace("http request completed")
	if resp.StatusCode != http.StatusCreated {
		return fmt.Errorf("unexpected status code: %d", resp.StatusCode)
	}
	return nil
}

func (c *Client) doDelete(relativeURL string) error {
	childCtx, cancel := context.WithCancel(c.ctx)
	defer cancel()

	req, err := http.NewRequestWithContext(childCtx, "DELETE", fmt.Sprintf("%s%s", c.opts.addr.String(), relativeURL), nil)
	if err != nil {
		return err
	}

	hash := sha256.Sum256([]byte(fmt.Sprintf("%s%s", c.opts.addr.String(), relativeURL)))
	reqHash := hex.EncodeToString(hash[:])

	l := c.logger.WithFields(logrus.Fields{"method": req.Method, "url": req.URL.String()})
	l.WithField("request_id", reqHash).Trace("sending http request")
	resp, err := c.inner.Do(req)
	if err != nil {
		return err
	}
	l.WithField("statusCode", resp.StatusCode).WithField("request_id", reqHash).Trace("http request completed")
	if resp.StatusCode != http.StatusNoContent {
		return fmt.Errorf("unexpected status code: %d", resp.StatusCode)
	}
	return nil
}

// LoadCodeletSet loads a codelet set to the jbpf device
func (c *Client) LoadCodeletSet(req *LoadCodeletSetRequest) error {
	return c.doPost("", req)
}

// UnloadCodeletSet unloads a codelet set from the jbpf device
func (c *Client) UnloadCodeletSet(codeletSetID string) error {
	return c.doDelete(fmt.Sprintf("/%s", codeletSetID))
}
