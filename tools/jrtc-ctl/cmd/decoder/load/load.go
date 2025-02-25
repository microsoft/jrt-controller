// Copyright (c) Microsoft Corporation. All rights reserved.

package load

import (
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/services/decoder"
	"errors"
	"fmt"
	"path/filepath"
	"strings"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
)

type runOptions struct {
	client  *decoder.ClientOptions
	general *common.GeneralOptions

	deviceID        uint16
	_fwdDestination string
	fwdDestination  jrtcbindings.Destination
	protoMsgName    string
	protoName       string
	protoPath       string
	streamName      string
	streamPath      string
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.Uint16Var(&opts.deviceID, "device-id", uint16(jrtcbindings.DeviceIDAny), "jrt-controller device ID")
	flags.StringVar(&opts._fwdDestination, "fwd-destination", jrtcbindings.DestinationAny.String(), "stream forward destination, must be one of: "+strings.Join(jrtcbindings.AllDestinationsStr, ", "))
	flags.StringVar(&opts.protoMsgName, "proto-msg-name", "", "protobuf message name")
	flags.StringVar(&opts.protoName, "proto-name", "", "protobuf package name")
	flags.StringVar(&opts.protoPath, "proto-path", "./", "path to protobuf files")
	flags.StringVar(&opts.streamName, "stream-name", "test-stream-name", "stream name")
	flags.StringVar(&opts.streamPath, "stream-path", "test-stream-path", "stream path")
}

func (o *runOptions) parse() error {
	fwdDest, err := jrtcbindings.DestinationFromString(o._fwdDestination)
	if err != nil {
		return err
	}
	o.fwdDestination = fwdDest

	if len(o.protoMsgName) == 0 {
		return fmt.Errorf("missing required flag: --proto-msg-name")
	}
	if len(o.protoName) == 0 {
		return fmt.Errorf("missing required flag: --proto-name")
	}

	return nil
}

// Command Load a schema to a local decoder
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		client:  decoder.EmptyClientOptions(),
		general: opts,
	}
	cmd := &cobra.Command{
		Use:   "load",
		Short: "Load a schema to a local decoder",
		Long:  "Load a schema to a local decoder",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	addToFlags(cmd.PersistentFlags(), runOptions)
	decoder.AddClientOptionsToFlags(cmd.PersistentFlags(), runOptions.client)
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := errors.Join(
		opts.general.Parse(),
		opts.client.Parse(),
		opts.parse(),
	); err != nil {
		return err
	}

	logger := opts.general.Logger

	client, err := decoder.NewClient(cmd.Context(), logger, opts.client)
	if err != nil {
		return err
	}

	compiledProtos, err := common.LoadFilesByGlob(logger, filepath.Join(opts.protoPath, fmt.Sprintf("%s.pb", opts.protoName)))
	if err != nil {
		return err
	}

	protoName := fmt.Sprintf("%s.pb", opts.protoName)
	proto, ok := compiledProtos[protoName]
	if !ok {
		return fmt.Errorf(`expected "%s" proto to be present`, protoName)
	}

	streamUUID, err := jrtcbindings.GenerateStreamID(
		opts.fwdDestination,
		uint8(opts.deviceID),
		&opts.streamPath,
		&opts.streamName,
	)
	if err != nil {
		return err
	}

	streamIDB, err := streamUUID.MarshalBinary()
	if err != nil {
		return err
	}

	return client.Load(map[string]*decoder.LoadRequest{
		opts.protoName: {
			CompiledProto: proto.Content,
			Streams: map[string][]byte{
				opts.protoMsgName: streamIDB,
			},
		},
	})
}
