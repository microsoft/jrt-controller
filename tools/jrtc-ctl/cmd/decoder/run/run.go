// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package run

import (
	"encoding/json"
	"errors"
	"fmt"
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/services/cache"
	"jrtc-ctl/services/decoder"
	"jrtc-ctl/services/loganalytics"
	"jrtc-ctl/services/udp"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"
	"google.golang.org/protobuf/proto"

	"google.golang.org/protobuf/encoding/protojson"
	"google.golang.org/protobuf/reflect/protodesc"
	"google.golang.org/protobuf/reflect/protoreflect"
	"google.golang.org/protobuf/types/descriptorpb"
	"google.golang.org/protobuf/types/dynamicpb"
)

type runOptions struct {
	cache         *cache.Options
	decoderServer *decoder.ServerOptions
	general       *common.GeneralOptions
	udpServer     *udp.ServerOptions
	uploader      *loganalytics.UploaderOptions
}

// Command Run decoder to collect, decode and print jrt-controller output
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		cache:         &cache.Options{},
		decoderServer: decoder.EmptyServerOptions(),
		general:       opts,
		udpServer:     udp.EmptyServerOptions(),
		uploader:      &loganalytics.UploaderOptions{},
	}
	cmd := &cobra.Command{
		Use:   "run",
		Short: "Run decoder to collect, decode and print jrt-controller output",
		Long:  "Run dynamic protobuf decoder to collect, decode and print jrt-controller output.",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	cache.AddToFlags(cmd.PersistentFlags(), runOptions.cache)
	decoder.AddServerOptionsToFlags(cmd.PersistentFlags(), runOptions.decoderServer)
	loganalytics.AddUploaderOptionsToFlags(cmd.PersistentFlags(), runOptions.uploader)
	udp.AddServerOptionsToFlags(cmd.PersistentFlags(), runOptions.udpServer, false)
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := errors.Join(
		opts.cache.Parse(),
		opts.decoderServer.Parse(),
		opts.general.Parse(),
		opts.uploader.Parse(),
		opts.udpServer.Parse(),
	); err != nil {
		return err
	}

	logger := opts.general.Logger

	cacheClient, err := cache.NewClient(cmd.Context(), logger, opts.cache)
	if err != nil {
		return err
	}

	uploader, err := loganalytics.NewUploader(cmd.Context(), logger, opts.uploader)
	if err != nil {
		return err
	}

	srv, err := decoder.NewServer(cmd.Context(), logger, opts.decoderServer, cacheClient, 1000, func(bs []byte) (string, error) {
		sid, err := jrtcbindings.StreamIDFromBytes(bs)
		if err != nil {
			return "", err
		}

		sid.Format(&jrtcbindings.StreamIDFormatConfig{
			ClearDeviceID:              true,
			ClearForwardingDestination: true,
			ClearVersion:               true,
		})

		return "streamToSchema/" + sid.String(), nil
	})
	if err != nil {
		return err
	}

	udpServer, err := udp.NewServer(cmd.Context(), logger, opts.udpServer, 1000)
	if err != nil {
		return err
	}

	g, ctx := errgroup.WithContext(cmd.Context())

	g.Go(func() error {
		activeServices := []bool{true, true}

		for {
			select {
			case recData, ok := <-srv.OutQ:
				if !ok {
					activeServices[0] = false
					if !activeServices[0] && !activeServices[1] {
						return nil
					} else {
						continue
					}
				}
				buf, err := attemptDecode(logger, srv, recData)
				if err != nil {
					logger.WithError(err).Error("error decoding and printing rec data")
					continue
				}
				if err := printAndUpload(logger, buf, uploader); err != nil {
					logger.WithError(err).Error("error handling rec data")
				}
			case buf, ok := <-udpServer.OutQ:
				if !ok {
					activeServices[0] = false
					if !activeServices[0] && !activeServices[1] {
						return nil
					} else {
						continue
					}
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

	g.Go(func() error {
		return udpServer.Serve()
	})

	return g.Wait()
}

func attemptDecode(logger *logrus.Logger, srv *decoder.Server, data *decoder.RecData) ([]byte, error) {
	streamUUID, err := jrtcbindings.StreamIDFromBytes(data.StreamUUID[:])
	if err != nil {
		return nil, err
	}
	l := logger.WithField("streamUUID", streamUUID.String())

	schema, exists, err := srv.StreamToSchema.Get(data.StreamUUID[:])
	if err != nil {
		return nil, err
	} else if !exists {
		err = fmt.Errorf("no schema found for stream UUID %s", streamUUID.String())
		l.WithError(err).Error("missing schema")
		return nil, err
	}

	sch, exist, err := srv.Schemas.Get(schema.ProtoPackage)
	if err != nil {
		return nil, err
	} else if !exist {
		return nil, fmt.Errorf("no schema found for proto package %s", schema.ProtoPackage)
	}

	fds := &descriptorpb.FileDescriptorSet{}
	if err := proto.Unmarshal(sch.ProtoDescriptor, fds); err != nil {
		return nil, err
	}

	pd, err := protodesc.NewFiles(fds)
	if err != nil {
		return nil, err
	}

	msgName := protoreflect.FullName(schema.ProtoMsg)
	var desc protoreflect.Descriptor
	desc, err = pd.FindDescriptorByName(msgName)
	if err != nil {
		l.WithError(err).Error("error finding descriptor by name")
		return nil, err
	}

	md, ok := desc.(protoreflect.MessageDescriptor)
	if !ok {
		err := fmt.Errorf("failed to cast desc to protoreflect.MessageDescriptor, got %T", desc)
		l.WithError(err).Error("internal error")
		return nil, err
	}

	msg := dynamicpb.NewMessage(md)

	err = proto.Unmarshal(data.Payload, msg)
	if err != nil {
		l.WithError(err).Error("error unmarshalling payload")
		return nil, err
	}

	res, err := protojson.Marshal(msg)
	if err != nil {
		l.WithError(err).Error("error marshalling message to JSON")
		return nil, err
	}

	return res, nil
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
