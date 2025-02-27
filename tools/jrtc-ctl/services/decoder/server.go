// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package decoder

import (
	context "context"
	"crypto/sha1"
	"jrtc-ctl/services/cache"
	"encoding/base64"
	"encoding/hex"
	"errors"
	"os"
	"os/signal"
	"path/filepath"
	"strings"
	"sync"
	"syscall"
	"time"

	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/descriptorpb"

	"golang.org/x/sync/errgroup"

	"github.com/sirupsen/logrus"
	"google.golang.org/grpc/codes"
	status "google.golang.org/grpc/status"
)

//go:generate protoc -I. -I../../3p/googleapis --go_out=paths=source_relative:. --go-grpc_out=paths=source_relative:. --grpc-gateway_out=logtostderr=true,paths=source_relative:. --experimental_allow_proto3_optional dynamicdecoder.proto

const (
	dataReadDeadline = 1 * time.Second
	decoderChanSize  = 100
)

// Server is a server that implements the DynamicDecoderServer interface
type Server struct {
	UnimplementedDynamicDecoderServer

	ctx            context.Context
	logger         *logrus.Logger
	opts           *ServerOptions
	outQ           chan<- *RecData
	Schemas        *cache.Store[string, RecordedProtoDescriptor]
	StreamToSchema *cache.Store[[]byte, RecordedStreamToSchema]
}

// RecData is a received data payload
type RecData struct {
	Payload    []byte
	StreamUUID [16]byte
}

// RecordedProtoDescriptor is a recorded proto descriptor
type RecordedProtoDescriptor struct {
	checksum        [20]byte
	ProtoDescriptor []byte
}

// RecordedStreamToSchema is a mapping of a stream to a schema
type RecordedStreamToSchema struct {
	ProtoMsg     string
	ProtoPackage string
}

// NewServer returns a new Server
func NewServer(parentCtx context.Context, logger *logrus.Logger, opts *ServerOptions, cacheClient *cache.Client, outQ chan<- *RecData, streamIDTransform func([]byte) (string, error)) (*Server, error) {
	ctx, cancelFunc := context.WithCancel(parentCtx)
	stopper := make(chan os.Signal, 1)
	signal.Notify(stopper, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		sig := <-stopper
		logger.WithField("sig", sig).Info("cancelling context due to signal")
		cancelFunc()
	}()

	schemas, err1 := cache.NewStore[string, RecordedProtoDescriptor](cacheClient, func(key string) (string, error) {
		return "schema/" + key, nil
	})
	streamToSchema, err2 := cache.NewStore[[]byte, RecordedStreamToSchema](cacheClient, streamIDTransform)
	if err := errors.Join(err1, err2); err != nil {
		return nil, err
	}

	return &Server{
		ctx:            ctx,
		logger:         logger,
		opts:           opts,
		Schemas:        schemas,
		outQ:           outQ,
		StreamToSchema: streamToSchema,
	}, nil
}

// Serve starts the server
func (s *Server) Serve() error {
	g, ctx := errgroup.WithContext(s.ctx)

	var wg sync.WaitGroup
	wg.Add(1)
	g.Go(func() error {
		return s.serveControl(ctx, &wg)
	})
	wg.Wait()
	if s.opts.gateway.enabled {
		g.Go(func() error {
			return s.serveGateway(ctx)
		})
	}
	if s.opts.dataEnabled {
		g.Go(func() error {
			return s.serveData(ctx)
		})
	}

	return g.Wait()
}

// UpsertProtoPackage registers a proto package with the server
func (s *Server) UpsertProtoPackage(_ context.Context, req *Schema) (*Response, error) {
	checksum := sha1.Sum(req.GetProtoDescriptor())
	checksumAsString := base64.StdEncoding.EncodeToString(checksum[:])

	fds := &descriptorpb.FileDescriptorSet{}
	if err := proto.Unmarshal(req.GetProtoDescriptor(), fds); err != nil {
		s.logger.WithError(err).Error("unable to unmarshal proto descriptor")
		return nil, err
	}

	protoPackageFile := fds.File[0].GetName()
	protoPackageName := strings.TrimSuffix(protoPackageFile, filepath.Ext(protoPackageFile))
	l := s.logger.WithFields(logrus.Fields{
		"protoPackageName": protoPackageName,
		"checksum":         checksumAsString,
	})

	if len(fds.File) != 1 {
		err := status.Errorf(codes.InvalidArgument, "expected exactly one file descriptor in the set, got %d", len(fds.File))
		l.WithError(err).Error("unable to interpret proto descriptor")
		return nil, err
	}

	if current, ok, err := s.Schemas.Get(protoPackageName); err != nil {
		l.WithError(err).Error("failed to get proto package")
		return nil, err
	} else if ok {
		if current.checksum == checksum {
			l.Info("checksum matches, skipping")
			return &Response{Status: ResponseStatus_STATUS_OK, Message: "Proto package already exists, checksum matches"}, nil
		}
		l.Warn("overwriting existing proto package")
	} else {
		l.Info("setting proto package")
	}

	if err := s.Schemas.Set(protoPackageName, &RecordedProtoDescriptor{
		checksum:        checksum,
		ProtoDescriptor: req.GetProtoDescriptor(),
	}); err != nil {
		l.WithError(err).Error("failed to set proto package")
		return nil, err
	}

	return &Response{Status: ResponseStatus_STATUS_OK}, nil
}

// AddStreamToSchemaAssociation associates a stream with a schema
func (s *Server) AddStreamToSchemaAssociation(_ context.Context, req *AddSchemaAssociation) (*Response, error) {
	streamID := req.GetStreamId()
	streamIDStr := make([]byte, hex.EncodedLen(len(streamID)))
	hex.Encode(streamIDStr, streamID)

	l := s.logger.WithFields(logrus.Fields{
		"protoMsg":     req.GetProtoMsg(),
		"protoPackage": req.GetProtoPackage(),
		"streamUUID":   string(streamIDStr),
	})

	if current, ok, err := s.StreamToSchema.Get(streamID); err != nil {
		l.WithError(err).Error("unable to get stream to schema association")
		return nil, err
	} else if ok {
		if current.ProtoMsg == req.GetProtoMsg() && current.ProtoPackage == req.GetProtoPackage() {
			return &Response{Status: ResponseStatus_STATUS_OK, Message: "stream already has a schema association"}, nil
		}
		err := status.Errorf(codes.AlreadyExists, "stream already has a schema association")
		l.WithError(err).Error("error adding stream to schema association")
		return nil, err
	}

	if _, ok, err := s.Schemas.Get(req.GetProtoPackage()); err != nil {
		l.WithError(err).Error("failed to get proto package")
		return nil, err
	} else if !ok {
		err := status.Errorf(codes.NotFound, "proto package %s not found", req.GetProtoPackage())
		l.WithError(err).Error("error adding stream to schema association")
		return nil, err
	}

	if err := s.StreamToSchema.Set(streamID, &RecordedStreamToSchema{
		ProtoMsg:     req.GetProtoMsg(),
		ProtoPackage: req.GetProtoPackage(),
	}); err != nil {
		l.WithError(err).Error("failed to set stream to schema association")
		return nil, err
	}

	l.Info("association added")

	return &Response{Status: ResponseStatus_STATUS_OK}, nil
}

// DeleteStreamToSchemaAssociation removes the association between a stream and a schema
func (s *Server) DeleteStreamToSchemaAssociation(_ context.Context, req *RemoveSchemaAssociation) (*Response, error) {
	streamID := req.GetStreamId()
	streamIDStr := make([]byte, hex.EncodedLen(len(streamID)))
	hex.Encode(streamIDStr, streamID)

	l := s.logger.WithField("streamUUID", string(streamIDStr))

	if current, ok, err := s.StreamToSchema.Get(streamID); err != nil {
		return nil, err
	} else if !ok {
		l.Debug("no association found for stream UUID")
	} else {
		if err := s.StreamToSchema.Delete(streamID); err != nil {
			return nil, err
		}
		l.WithFields(logrus.Fields{
			"protoMsg":     current.ProtoMsg,
			"protoPackage": current.ProtoPackage,
		}).Info("association removed")
	}

	return &Response{Status: ResponseStatus_STATUS_OK}, nil
}
