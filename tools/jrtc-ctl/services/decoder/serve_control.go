package decoder

import (
	"context"
	"fmt"
	"net"
	"sync"

	grpc "google.golang.org/grpc"
)

func (s *Server) serveControl(ctx context.Context, wg *sync.WaitGroup) error {
	var serverOpts []grpc.ServerOption
	grpcServer := grpc.NewServer(serverOpts...)
	RegisterDynamicDecoderServer(grpcServer, s)

	s.logger.Trace("starting gRPC control server")
	defer s.logger.Trace("closing gRPC control server")

	control, err := net.Listen(controlScheme, fmt.Sprintf("%s:%d", s.opts.control.ip, s.opts.control.port))
	if err != nil {
		return err
	}

	s.logger.WithField("addr", control.Addr().Network()+"://"+control.Addr().String()).Debug("starting control server")
	defer func() {
		s.logger.WithField("addr", control.Addr().Network()+"://"+control.Addr().String()).Debug("stopping control server")
		if err := control.Close(); err != nil {
			if opErr, ok := err.(*net.OpError); !ok || opErr.Err.Error() != "use of closed network connection" {
				s.logger.WithError(err).Errorf("error closing control server")
			}
		}
	}()

	// gracefully stop
	go func() {
		<-ctx.Done()
		grpcServer.GracefulStop()
	}()

	wg.Done()
	return grpcServer.Serve(control)
}
