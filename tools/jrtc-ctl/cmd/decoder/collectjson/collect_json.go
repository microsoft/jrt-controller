// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package collectjson

import (
	"encoding/json"
	"errors"
	"jrtc-ctl/common"
	"jrtc-ctl/services/loganalytics"
	"jrtc-ctl/services/udp"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"
)

type collectJSONOptions struct {
	general   *common.GeneralOptions
	udpServer *udp.ServerOptions
	uploader  *loganalytics.UploaderOptions
}

// Command CollectJSON to collect JSON data and print/upload
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &collectJSONOptions{
		general:   opts,
		udpServer: udp.EmptyServerOptions(),
		uploader:  &loganalytics.UploaderOptions{},
	}
	cmd := &cobra.Command{
		Use:   "collectjson",
		Short: "Collect JSON data and print/upload",
		Long:  "Collect JSON data over UDP socket and print & upload.",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	loganalytics.AddUploaderOptionsToFlags(cmd.PersistentFlags(), runOptions.uploader)
	udp.AddServerOptionsToFlags(cmd.PersistentFlags(), runOptions.udpServer, true)
	return cmd
}

func run(cmd *cobra.Command, opts *collectJSONOptions) error {
	if err := errors.Join(
		opts.general.Parse(),
		opts.uploader.Parse(),
		opts.udpServer.Parse(),
	); err != nil {
		return err
	}

	logger := opts.general.Logger

	uploader, err := loganalytics.NewUploader(cmd.Context(), logger, opts.uploader)
	if err != nil {
		return err
	}

	srv, err := udp.NewServer(cmd.Context(), logger, opts.udpServer, 1000)
	if err != nil {
		return err
	}

	g, ctx := errgroup.WithContext(cmd.Context())

	g.Go(func() error {
		for {
			select {
			case buf, ok := <-srv.OutQ:
				if !ok {
					return nil
				}
				if err := validateJSON(logger, buf); err != nil {
					logger.WithError(err).Error("received invalid JSON data")
					continue
				}
				if err := printAndUpload(logger, buf, uploader); err != nil {
					logger.WithError(err).Error("error handling JSON data")
				}
			case <-ctx.Done():
				return nil
			}
		}
	})

	g.Go(func() error {
		return srv.Serve()
	})

	return g.Wait()
}

func validateJSON(logger *logrus.Logger, res []byte) error {
	var out interface{}
	if err := json.Unmarshal(res, &out); err != nil {
		logger.WithError(err).Error("error unmarshalling JSON")
		return err
	}
	return nil
}

func printAndUpload(logger *logrus.Logger, res []byte, uploader *loganalytics.Uploader) error {
	if uploader != nil {
		if err := uploader.Upload(res); err != nil {
			logger.WithError(err).Error("error uploading message")
			return err
		}
	}

	logger.Infof("REC: %s", string(res))

	return nil
}
