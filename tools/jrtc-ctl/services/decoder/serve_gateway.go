// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
package decoder

import (
	"context"
	"fmt"
	"net/http"

	"github.com/grpc-ecosystem/grpc-gateway/v2/runtime"
	grpc "google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

func (s *Server) serveGateway(ctx context.Context) error {
	mux := runtime.NewServeMux()
	opts := []grpc.DialOption{grpc.WithTransportCredentials(insecure.NewCredentials())}
	err := RegisterDynamicDecoderHandlerFromEndpoint(ctx, mux, fmt.Sprintf("%s:%d", s.opts.control.ip, s.opts.control.port), opts)
	if err != nil {
		return err
	}

	srv := &http.Server{Addr: fmt.Sprintf("%s:%d", s.opts.gateway.ip, s.opts.gateway.port), Handler: mux}
	s.logger.WithField("addr", srv.Addr).Debug("starting gateway server")
	defer func() {
		s.logger.WithField("addr", srv.Addr).Debug("stopping gateway server")
	}()

	// gracefully stop
	go func() {
		<-ctx.Done()
		err := srv.Close()
		if err != nil {
			s.logger.WithError(err).Error("error shutting down gateway server")
		}
	}()

	if err := srv.ListenAndServe(); err != http.ErrServerClosed {
		return err
	}
	return nil
}
