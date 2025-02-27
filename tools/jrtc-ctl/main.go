// Copyright (c) Microsoft Corporation. All rights reserved.

package main

import (
	"context"
	"jrtc-ctl/cmd/app"
	"jrtc-ctl/cmd/decoder"
	"jrtc-ctl/cmd/jbpf"
	"jrtc-ctl/cmd/load"
	"jrtc-ctl/cmd/streamid"
	"jrtc-ctl/cmd/unload"
	"jrtc-ctl/common"
	"os"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
)

func main() {
	if err := cli().ExecuteContext(context.Background()); err != nil {
		logrus.WithError(err).Fatal("Exiting")
	}
}

func cli() *cobra.Command {
	cmd := &cobra.Command{
		Use:  os.Args[0],
		Long: "Command line tool to manage jrt-controller components and deployments.",
	}
	opts := common.NewGeneralOptions(cmd.PersistentFlags())
	cmd.AddCommand(
		app.Command(opts),
		decoder.Command(opts),
		jbpf.Command(opts),
		load.Command(opts),
		streamid.Command(opts),
		unload.Command(opts),
	)
	return cmd
}
