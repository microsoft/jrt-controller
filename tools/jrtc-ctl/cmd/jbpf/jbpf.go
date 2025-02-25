// Copyright (c) Microsoft Corporation. All rights reserved.

package jbpf

import (
	"jrtc-ctl/cmd/jbpf/load"
	"jrtc-ctl/cmd/jbpf/unload"
	"jrtc-ctl/common"

	"github.com/spf13/cobra"
)

// Command returns the jbpf commands
func Command(opts *common.GeneralOptions) *cobra.Command {
	cmd := &cobra.Command{
		Use:   "jbpf",
		Long:  "Execute a JBPF subcommand.",
		Short: "Execute a JBPF subcommand",
	}

	cmd.AddCommand(
		load.Command(opts),
		unload.Command(opts),
	)
	return cmd
}
