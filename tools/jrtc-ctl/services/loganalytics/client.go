package loganalytics

import (
	"bytes"
	"context"
	"crypto/hmac"
	"crypto/sha256"
	"encoding/base64"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
)

const (
	contentType       = "application/json"
	primaryKeyEnvVar  = "LA_PRIMARY_KEY"
	workspaceIDEnvVar = "LA_WORKSPACE_ID"
)

type client struct {
	ctx         context.Context
	inner       *http.Client
	keyBytes    []byte
	logger      *logrus.Logger
	opts        *options
	workspaceID string
}

func newClient(ctx context.Context, logger *logrus.Logger, opts *options) (*client, error) {
	workspaceID := os.Getenv(workspaceIDEnvVar)
	primaryKey := os.Getenv(primaryKeyEnvVar)
	if primaryKey == "" || workspaceID == "" {
		return nil, fmt.Errorf("%s and %s must be set", workspaceIDEnvVar, primaryKeyEnvVar)
	}
	keyBytes, err := base64.StdEncoding.DecodeString(primaryKey)
	if err != nil {
		return nil, err
	}
	return &client{
		ctx:         ctx,
		inner:       &http.Client{},
		keyBytes:    keyBytes,
		logger:      logger,
		opts:        opts,
		workspaceID: workspaceID,
	}, nil
}

func (c *client) buildSignature(message string) (string, error) {
	mac := hmac.New(sha256.New, c.keyBytes)
	mac.Write([]byte(message))
	return base64.StdEncoding.EncodeToString(mac.Sum(nil)), nil
}

func (c *client) send(data []byte) (err error) {
	dateString := time.Now().UTC().Format(time.RFC1123)
	dateString = strings.Replace(dateString, "UTC", "GMT", -1)

	stringToHash := fmt.Sprintf("POST\n%d\napplication/json\nx-ms-date:%s\n/api/logs", len(data), dateString)
	hashedString, err := c.buildSignature(stringToHash)
	if err != nil {
		c.logger.WithError(err).Error("Invalid signature")
		return err
	}

	signature := fmt.Sprintf("SharedKey %s:%s", c.workspaceID, hashedString)
	url := fmt.Sprintf("https://%s.ods.opinsights.azure.com/api/logs?api-version=2016-04-01", c.workspaceID)

	req, err := http.NewRequestWithContext(c.ctx, "POST", url, bytes.NewReader(data))
	if err != nil {
		c.logger.WithError(err).Error("Invalid request")
		return err
	}

	req.Header.Add("Log-Type", c.opts.logType)
	req.Header.Add("Authorization", signature)
	req.Header.Add("Content-Type", contentType)
	req.Header.Add("x-ms-date", dateString)
	if c.opts.timeGeneratedField != "" {
		req.Header.Add("time-generated-field", c.opts.timeGeneratedField)
	}

	resp, err := c.inner.Do(req)
	if err != nil {
		c.logger.WithError(err).Error("Request failed ")
		return err
	}
	if err := resp.Body.Close(); err != nil {
		c.logger.WithError(err).Error("Failed to close response body")
		return err
	}
	if resp.StatusCode != 200 {
		bodyBytes, _ := io.ReadAll(resp.Body)
		bodyString := string(bodyBytes)
		c.logger.WithField("response_body", bodyString).WithError(err).Error("Non 200 response")
		return fmt.Errorf("received non-200 response: %d", resp.StatusCode)
	}

	return
}
