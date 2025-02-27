// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
package decoder

import (
	"context"
	"encoding/hex"
	"errors"
	"fmt"
	"net"
	"time"
)

func (s *Server) serveData(ctx context.Context) error {
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
		case <-ctx.Done():
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

			bufferHex := hex.EncodeToString(buffer[:n])
			fmt.Printf("recv \"%s\"(%d)\n", bufferHex, n)

			if n < 16 {
				s.logger.Warn("received data is less than 18 bytes, skipping")
				continue
			}
			streamUUID := [16]byte{}
			copy(streamUUID[:], buffer[:16])
			payload := buffer[16:n]

			select {
			case s.outQ <- &RecData{Payload: payload, StreamUUID: streamUUID}:
			default:
				s.logger.Warn("outQ is full, dropping data")
			}
		}
	}
}
