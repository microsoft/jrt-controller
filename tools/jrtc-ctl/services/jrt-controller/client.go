// Copyright (c) Microsoft Corporation. All rights reserved.

package jrtc

import (
	"context"
	"fmt"
	"io"
	"net/http"

	"github.com/sirupsen/logrus"
)

//go:generate go run github.com/oapi-codegen/oapi-codegen/v2/cmd/oapi-codegen --config=config.yaml ../../../../src/rest_api_lib/openapi.json

// JrtcAPIClient is a wrapped Client with logging
//revive:disable-next-line
type JrtcAPIClient struct {
	ctx    context.Context
	inner  *ClientWithResponses
	logger *logrus.Logger
}

// NewJrtcAPIClient creates a new JrtcAPIClient
func NewJrtcAPIClient(ctx context.Context, logger *logrus.Logger, opts *Options) (*JrtcAPIClient, error) {
	inner, err := NewClientWithResponses(opts.addr)
	if err != nil {
		return nil, err
	}

	return &JrtcAPIClient{
		ctx:    ctx,
		inner:  inner,
		logger: logger,
	}, nil
}

// GetAppWithResponse returns a loaded app
func (c *JrtcAPIClient) GetAppWithResponse(id int32, reqEditors ...RequestEditorFn) (*GetAppResponse, error) {
	resp, err := c.inner.GetAppWithResponse(c.ctx, id, reqEditors...)
	if err != nil {
		return nil, err
	}
	return logResponse(c.logger, resp, resp.HTTPResponse, err)
}

// GetAppsWithResponse returns loaded apps
func (c *JrtcAPIClient) GetAppsWithResponse(reqEditors ...RequestEditorFn) (*GetAppsResponse, error) {
	resp, err := c.inner.GetAppsWithResponse(c.ctx, reqEditors...)
	if err != nil {
		return nil, err
	}
	return logResponse(c.logger, resp, resp.HTTPResponse, err)
}

// LoadAppWithBodyWithResponse loads an app with a typed response
func (c *JrtcAPIClient) LoadAppWithBodyWithResponse(contentType string, body io.Reader, reqEditors ...RequestEditorFn) (*LoadAppResponse, error) {
	resp, err := c.inner.LoadAppWithBodyWithResponse(c.ctx, contentType, body, reqEditors...)
	if err != nil {
		return nil, err
	}
	return logResponse(c.logger, resp, resp.HTTPResponse, err)
}

// LoadAppWithResponse loads an app
func (c *JrtcAPIClient) LoadAppWithResponse(body LoadAppJSONRequestBody, reqEditors ...RequestEditorFn) (*LoadAppResponse, error) {
	resp, err := c.inner.LoadAppWithResponse(c.ctx, body, reqEditors...)
	if err != nil {
		return nil, err
	}
	return logResponse(c.logger, resp, resp.HTTPResponse, err)
}

// UnloadAppWithResponse unloads an app
func (c *JrtcAPIClient) UnloadAppWithResponse(id int32, reqEditors ...RequestEditorFn) (*UnloadAppResponse, error) {
	resp, err := c.inner.UnloadAppWithResponse(c.ctx, id, reqEditors...)
	if err != nil {
		return nil, err
	}
	return logResponse(c.logger, resp, resp.HTTPResponse, err)
}

type someResponse interface {
	Status() string
	StatusCode() int
}

func logResponse[T someResponse](logger *logrus.Logger, resp T, rawResp *http.Response, err error) (T, error) {
	l := logger.WithFields(logrus.Fields{
		"address": rawResp.Request.URL.String(),
	})
	l.Info("new http request")
	l = l.WithFields(logrus.Fields{
		"code":   rawResp.StatusCode,
		"method": rawResp.Request.Method,
		"status": rawResp.Status,
	})

	if err != nil {
		l.WithError(err).Error("failed to send http request")
		return resp, err
	}
	if resp.StatusCode() < 200 || resp.StatusCode() > 299 {
		l.Error("failed to send http request with non 2XX status code")
		return resp, fmt.Errorf("failed to send http request with non 2XX status code")
	}
	l.Info("successfully sent http request")
	return resp, err
}
