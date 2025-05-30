// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package decoder

import (
	"jrtc-ctl/cmd/decoder/collectjson"
	"jrtc-ctl/cmd/decoder/load"
	"jrtc-ctl/cmd/decoder/run"
	"jrtc-ctl/cmd/decoder/unload"
	"jrtc-ctl/common"

	"github.com/spf13/cobra"
)

// Command returns the decoder commands
func Command(opts *common.GeneralOptions) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "decoder",
		Long:  "Execute a decoder subcommand.",
		Short: "Execute a decoder subcommand",
	}
	cmd.AddCommand(
		collectjson.Command(opts),
		load.Command(opts),
		run.Command(opts),
		unload.Command(opts),
	)
	return cmd
}
