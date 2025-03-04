// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package streamid

import (
	"fmt"
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"strings"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
)

type runOptions struct {
	forwardDestination string
	deviceID           uint16
	streamPath         string
	streamName         string
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.StringVarP(&opts.forwardDestination, "forward-destination", "d", jrtcbindings.DefaultDestination.String(), "stream forward destination, must be one of: "+strings.Join(jrtcbindings.AllDestinationsStr, ", "))
	flags.Uint16VarP(&opts.deviceID, "device-id", "D", uint16(jrtcbindings.DeviceIDAny), "device ID")
	flags.StringVarP(&opts.streamPath, "stream-path", "p", "", `stream path, of the form "deployment://type{/scope}+"`)
	flags.StringVarP(&opts.streamName, "stream-name", "n", "", "stream name")
}

// Command Generate stream id
func Command(_ *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{}
	cmd := &cobra.Command{
		Use:   "streamid",
		Short: "Generate stream ID",
		Long:  "Generate stream ID compliant with the jrt-controller platform",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	addToFlags(cmd.Flags(), runOptions)
	return cmd
}

func run(_ *cobra.Command, opts *runOptions) error {
	fwdDestination, err := jrtcbindings.DestinationFromString(opts.forwardDestination)
	if err != nil {
		return err
	}
	deviceID := uint8(opts.deviceID)
	streamPath := (*string)(nil)
	if len(opts.streamPath) != 0 {
		streamPath = &opts.streamPath
	}
	streamName := (*string)(nil)
	if len(opts.streamName) != 0 {
		streamName = &opts.streamName
	}
	streamID, err := jrtcbindings.GenerateStreamID(fwdDestination, deviceID, streamPath, streamName)
	if err != nil {
		return err
	}
	fmt.Println(streamID)
	return nil
}
