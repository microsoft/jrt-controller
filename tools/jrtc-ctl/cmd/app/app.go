// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package app

import (
	"jrtc-ctl/cmd/app/load"
	"jrtc-ctl/cmd/app/unload"
	"jrtc-ctl/common"

	"github.com/spf13/cobra"
)

// Command returns the app commands
func Command(opts *common.GeneralOptions) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "app",
		Long:  "Execute a jrt-controller application subcommand.",
		Short: "Execute a jrt-controller application subcommand",
	}

	cmd.AddCommand(
		load.Command(opts),
		unload.Command(opts),
	)
	return cmd
}
