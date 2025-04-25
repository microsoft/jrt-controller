// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package run

import (
	"errors"
	"fmt"
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/services/cache"
	"jrtc-ctl/services/decoder"
	"jrtc-ctl/services/loganalytics"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
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
	uploader      *loganalytics.UploaderOptions
}

// Command Run decoder to collect, decode and print jrt-controller output
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		cache:         &cache.Options{},
		decoderServer: decoder.EmptyServerOptions(),
		general:       opts,
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
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := errors.Join(
		opts.cache.Parse(),
		opts.decoderServer.Parse(),
		opts.general.Parse(),
		opts.uploader.Parse(),
	); err != nil {
		return err
	}

	logger := opts.general.Logger

	cacheClient, err := cache.NewClient(cmd.Context(), logger, opts.cache)
	if err != nil {
		return err
	}

	dataQ := make(chan *decoder.RecData, 1000)

	uploader, err := loganalytics.NewUploader(cmd.Context(), logger, opts.uploader)
	if err != nil {
		return err
	}

	srv, err := decoder.NewServer(cmd.Context(), logger, opts.decoderServer, cacheClient, dataQ, func(bs []byte) (string, error) {
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
	go func() {
		for {
			select {
			case recData := <-dataQ:
				if err := attemptDecodeAndPrint(logger, srv, recData, uploader); err != nil {
					logger.WithError(err).Error("error decoding and printing rec data")
				}
			case <-cmd.Context().Done():
				return
			}
		}

	}()
	return srv.Serve()
}

func attemptDecodeAndPrint(logger *logrus.Logger, srv *decoder.Server, data *decoder.RecData, uploader *loganalytics.Uploader) error {
	streamUUID, err := jrtcbindings.StreamIDFromBytes(data.StreamUUID[:])
	if err != nil {
		return err
	}
	l := logger.WithField("streamUUID", streamUUID.String())

	schema, exists, err := srv.StreamToSchema.Get(data.StreamUUID[:])
	if err != nil {
		return err
	} else if !exists {
		err = fmt.Errorf("no schema found for stream UUID %s", streamUUID.String())
		l.WithError(err).Error("missing schema")
		return err
	}

	sch, exist, err := srv.Schemas.Get(schema.ProtoPackage)
	if err != nil {
		return err
	} else if !exist {
		return fmt.Errorf("no schema found for proto package %s", schema.ProtoPackage)
	}

	fds := &descriptorpb.FileDescriptorSet{}
	if err := proto.Unmarshal(sch.ProtoDescriptor, fds); err != nil {
		return err
	}

	pd, err := protodesc.NewFiles(fds)
	if err != nil {
		return err
	}

	msgName := protoreflect.FullName(schema.ProtoMsg)
	var desc protoreflect.Descriptor
	desc, err = pd.FindDescriptorByName(msgName)
	if err != nil {
		l.WithError(err).Error("error finding descriptor by name")
		return err
	}

	md, ok := desc.(protoreflect.MessageDescriptor)
	if !ok {
		err := fmt.Errorf("failed to cast desc to protoreflect.MessageDescriptor, got %T", desc)
		l.WithError(err).Error("internal error")
		return err
	}

	msg := dynamicpb.NewMessage(md)

	err = proto.Unmarshal(data.Payload, msg)
	if err != nil {
		l.WithError(err).Error("error unmarshalling payload")
		return err
	}

	res, err := protojson.Marshal(msg)
	if err != nil {
		l.WithError(err).Error("error marshalling message to JSON")
		return err
	}

	if uploader != nil {
		if err := uploader.Upload(res); err != nil {
			l.WithError(err).Error("error uploading message")
			return err
		}
	}

	l.Infof("REC: %s", string(res))
	return nil
}
