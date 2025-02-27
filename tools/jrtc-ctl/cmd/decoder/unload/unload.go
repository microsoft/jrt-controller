// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package unload

import (
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/services/decoder"
	"errors"
	"fmt"
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
	streamName      string
	streamPath      string
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.Uint16Var(&opts.deviceID, "device-id", uint16(jrtcbindings.DeviceIDAny), "jrt-controller device ID")
	flags.StringVar(&opts._fwdDestination, "fwd-destination", jrtcbindings.DestinationAny.String(), "stream forward destination, must be one of: "+strings.Join(jrtcbindings.AllDestinationsStr, ", "))
	flags.StringVar(&opts.streamName, "stream-name", "", "stream name")
	flags.StringVar(&opts.streamPath, "stream-path", "", "stream path")
}

func (o *runOptions) parse() error {
	fwdDest, err := jrtcbindings.DestinationFromString(o._fwdDestination)
	if err != nil {
		return err
	}
	o.fwdDestination = fwdDest

	if len(o.streamName) == 0 {
		return fmt.Errorf("missing required flag: --stream-name")
	}
	if len(o.streamPath) == 0 {
		return fmt.Errorf("missing required flag: --stream-path")
	}

	return nil
}

// Command Unload a schema from a local decoder
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		client:  decoder.EmptyClientOptions(),
		general: opts,
	}
	cmd := &cobra.Command{
		Use:   "unload",
		Short: "Unload a schema from a local decoder",
		Long:  "Unload a schema from a local decoder",
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

	return client.Unload([][]byte{streamIDB})
}
