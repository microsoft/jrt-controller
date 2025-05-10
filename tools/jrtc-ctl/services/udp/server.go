// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package udp

import (
	context "context"
	"errors"
	"fmt"
	"net"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/sirupsen/logrus"
)

const (
	dataReadDeadline = 1 * time.Second
	decoderChanSize  = 100
)

// Server is a basic UDP server that listens for incoming data
type Server struct {
	ctx    context.Context
	logger *logrus.Logger
	opts   *ServerOptions
	OutQ   chan []byte
}

// NewServer returns a new Server
func NewServer(parentCtx context.Context, logger *logrus.Logger, opts *ServerOptions, qSize int) (*Server, error) {
	ctx, cancelFunc := context.WithCancel(parentCtx)
	stopper := make(chan os.Signal, 1)
	signal.Notify(stopper, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		sig := <-stopper
		logger.WithField("sig", sig).Info("cancelling context due to signal")
		cancelFunc()
	}()

	return &Server{
		ctx:    ctx,
		logger: logger,
		opts:   opts,
		OutQ:   make(chan []byte, qSize),
	}, nil
}

// Serve starts the server
func (s *Server) Serve() error {
	if !s.opts.dataEnabled {
		return nil
	}

	data, err := net.ListenPacket(dataScheme, fmt.Sprintf("%s:%d", s.opts.dataIP, s.opts.dataPort))
	if err != nil {
		return err
	}
	s.logger.WithField("addr", data.LocalAddr().Network()+"://"+data.LocalAddr().String()).Debug("starting data server")
	defer func() {
		s.logger.WithField("addr", data.LocalAddr().Network()+"://"+data.LocalAddr().String()).Debug("stopping data server")
		if err := data.Close(); err != nil {
			s.logger.WithError(err).Errorf("error closing data server")
		}
	}()

	for {
		select {
		case <-s.ctx.Done():
			close(s.OutQ)
			return nil
		default:
			buffer := make([]byte, s.opts.dataBufferSize)
			if err := data.SetReadDeadline(time.Now().Add(dataReadDeadline)); err != nil {
				return err
			}
			n, _, err := data.ReadFrom(buffer)
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				continue
			}
			if err != nil {
				return errors.Join(err, fmt.Errorf("error reading from UDP socket"))
			}

			select {
			case s.OutQ <- buffer[:n]:
			default:
				s.logger.Warn("outQ is full, dropping data")
			}
		}
	}
}
