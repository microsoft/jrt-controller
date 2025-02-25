// Copyright (c) Microsoft Corporation. All rights reserved.

package decoder

import (
	"bytes"
	"context"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"

	"github.com/google/uuid"
	"github.com/sirupsen/logrus"
	grpc "google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

const (
	apiVersion = `/v1`
)

// Client encapsulates the decoder client
type Client struct {
	baseURL    string
	conn       *grpc.ClientConn
	ctx        context.Context
	grpcClient *DynamicDecoderClient
	httpClient *http.Client
	logger     *logrus.Logger
	useGateway bool
}

// NewClient creates a new DynamicDecoderClient
func NewClient(ctx context.Context, logger *logrus.Logger, opts *ClientOptions) (*Client, error) {
	if !opts.enabled {
		return nil, nil
	}

	if opts.useGateway {
		baseURL := fmt.Sprintf("%s://%s:%d%s", controlGatewayScheme, opts.control.ip, opts.control.port, opts.httpRelativePath)
		return &Client{
			baseURL:    baseURL,
			ctx:        ctx,
			httpClient: &http.Client{},
			logger:     logger,
			useGateway: true,
		}, nil
	}

	conn, err := grpc.NewClient(
		fmt.Sprintf("%s:%d", opts.control.ip, opts.control.port),
		grpc.WithTransportCredentials(insecure.NewCredentials()),
	)
	if err != nil {
		return nil, err
	}

	client := NewDynamicDecoderClient(conn)

	return &Client{
		conn:       conn,
		ctx:        ctx,
		grpcClient: &client,
		logger:     logger,
		useGateway: false,
	}, nil
}

// Close closes the client connection
func (c *Client) Close() error {
	if c.conn != nil {
		return c.conn.Close()
	}
	return nil
}

// LoadRequest is a request to load a schema and stream
type LoadRequest struct {
	CompiledProto []byte
	Streams       map[string][]byte
}

// IntermediateReply is the return type from decoder
type IntermediateReply struct {
	Message string
	Status  string
}

func (c *Client) httpResponseParser(l *logrus.Entry, resp *http.Response) (*Response, error) {
	if resp.StatusCode < 200 || resp.StatusCode > 299 {
		err := fmt.Errorf("received non 2XX status code, %d", resp.StatusCode)
		l.WithError(err).Error("http request failed")
		return nil, err
	}

	buf, err := io.ReadAll(resp.Body)
	if err != nil {
		l.WithError(err).Error("failed to read http response")
		return nil, err
	}

	l.WithField("body", string(buf)).Trace("received response")

	var intermediateReply IntermediateReply
	if err := json.Unmarshal(buf, &intermediateReply); err != nil {
		l.WithError(err).Error("failed to unmarshal response")
		return nil, err
	}

	return &Response{
		Message: intermediateReply.Message,
		Status:  ResponseStatus(ResponseStatus_value[intermediateReply.Status]),
	}, nil
}

func (c *Client) doPut(relativeURL string, body any) (*Response, error) {
	childCtx, cancel := context.WithCancel(c.ctx)
	defer cancel()

	bodyB, err := json.Marshal(body)
	if err != nil {
		return nil, err
	}

	req, err := http.NewRequestWithContext(childCtx, "PUT", fmt.Sprintf("%s%s", c.baseURL, relativeURL), bytes.NewBuffer(bodyB))
	if err != nil {
		return nil, err
	}
	req.Header.Set("Content-Type", "application/json")

	l := c.logger.WithFields(logrus.Fields{"method": req.Method, "url": req.URL.String()})
	l.WithField("body", string(bodyB)).Trace("sending http request")
	resp, err := c.httpClient.Do(req)
	if err != nil {
		return nil, err
	}
	l.WithField("statusCode", resp.StatusCode).Trace("http request completed")
	return c.httpResponseParser(l, resp)
}

func (c *Client) doDelete(relativeURL string) (*Response, error) {
	childCtx, cancel := context.WithCancel(c.ctx)
	defer cancel()

	req, err := http.NewRequestWithContext(childCtx, "DELETE", fmt.Sprintf("%s%s", c.baseURL, relativeURL), nil)
	if err != nil {
		return nil, err
	}

	l := c.logger.WithFields(logrus.Fields{"method": req.Method, "url": req.URL.String()})
	l.Trace("sending http request")
	resp, err := c.httpClient.Do(req)
	if err != nil {
		return nil, err
	}
	l.WithField("statusCode", resp.StatusCode).Trace("http request completed")
	return c.httpResponseParser(l, resp)
}

func (c *Client) upsertProtoPackage(in *Schema) (*Response, error) {
	if c.useGateway {
		return c.doPut(fmt.Sprintf("%s/schema", apiVersion), in)
	}
	return (*c.grpcClient).UpsertProtoPackage(c.ctx, in)
}

func (c *Client) addStreamToSchemaAssociation(in *AddSchemaAssociation) (*Response, error) {
	if c.useGateway {
		return c.doPut(fmt.Sprintf("%s/stream", apiVersion), in)
	}
	return (*c.grpcClient).AddStreamToSchemaAssociation(c.ctx, in)
}

func (c *Client) deleteStreamToSchemaAssociation(in *RemoveSchemaAssociation) (*Response, error) {
	if c.useGateway {
		return c.doDelete(fmt.Sprintf("%s/stream?streamId=%s==", apiVersion, base64.RawURLEncoding.EncodeToString(in.StreamId)))
	}
	return (*c.grpcClient).DeleteStreamToSchemaAssociation(c.ctx, in)
}

// Load loads the schemas into the decoder
func (c *Client) Load(schemas map[string]*LoadRequest) error {
	for protoPackageName, req := range schemas {
		l := c.logger.WithFields(logrus.Fields{"pkg": protoPackageName})

		resp, err := c.upsertProtoPackage(&Schema{ProtoDescriptor: req.CompiledProto})
		if len(resp.GetMessage()) > 0 {
			l.WithField("message", resp.GetMessage()).Debug("got message from upsert proto package")
		}
		if err != nil {
			l.WithError(err).Error("failed to upsert proto package")
			return err
		}
		if resp.GetStatus() != ResponseStatus_STATUS_OK {
			l.Error("got failed status from upsert proto package")
			return fmt.Errorf("failed to upsert proto package %s", protoPackageName)
		}
		l.Info("successfully upserted proto package")

		for protoMsg, streamID := range req.Streams {
			streamUUID, err := uuid.FromBytes(streamID)
			if err != nil {
				l.WithError(err).Error("failed to parse stream ID")
				return err
			}
			resp, err := c.addStreamToSchemaAssociation(&AddSchemaAssociation{
				StreamId:     streamID,
				ProtoPackage: protoPackageName,
				ProtoMsg:     protoMsg,
			})
			sl := l.WithFields(logrus.Fields{"streamId": streamUUID, "protoMsg": protoMsg})
			if len(resp.GetMessage()) > 0 {
				sl.WithField("message", resp.GetMessage()).Debug("got message from upsert proto package")
			}
			if err != nil {
				sl.WithError(err).Error("failed to associate stream ID with proto package")
				return err
			}
			if resp.GetStatus() != ResponseStatus_STATUS_OK {
				sl.Error("got failed status when associating stream ID")
				return fmt.Errorf("got failed status when associating stream ID")
			}
			sl.Info("successfully associated stream ID with proto package")
		}
	}

	return nil
}

// Unload removes the stream association from the decoder
func (c *Client) Unload(streamIDBs [][]byte) error {
	for _, streamIDB := range streamIDBs {
		streamUUID, err := uuid.FromBytes(streamIDB)
		if err != nil {
			return err
		}
		fields := logrus.Fields{"streamId": streamUUID}
		resp, err := c.deleteStreamToSchemaAssociation(&RemoveSchemaAssociation{StreamId: streamIDB})
		if len(resp.GetMessage()) > 0 {
			fields["message"] = resp.GetMessage()
		}
		if err != nil {
			c.logger.WithFields(fields).WithError(err).Error("failed to disassociate stream ID with proto package")
			return err
		}
		if resp.GetStatus() != ResponseStatus_STATUS_OK {
			c.logger.WithFields(fields).Error("got failed status when disassociating stream ID")
			return fmt.Errorf("got failed status when disassociating stream ID")
		}
		c.logger.WithFields(fields).Info("successfully disassociated stream ID with proto package")
	}

	return nil
}
