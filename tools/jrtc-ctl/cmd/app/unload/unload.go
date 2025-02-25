// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package unload

import (
	"jrtc-ctl/common"
	"jrtc-ctl/services/jrt-controller"
	"errors"
	"fmt"
	"time"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
	"golang.org/x/sync/errgroup"
)

type runOptions struct {
	general    *common.GeneralOptions
	controller *jrtc.Options

	appName string
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.StringVar(&opts.appName, "app-name", "", "app name inside jrt-controller")
}

func (o *runOptions) parse() error {
	if len(o.appName) == 0 {
		return fmt.Errorf("--app-name is required")
	}

	return nil
}

// Command Unload a jrt-controller application
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		general:    opts,
		controller: &jrtc.Options{},
	}
	cmd := &cobra.Command{
		Use:   "unload",
		Short: "Unload a jrt-controller application",
		Long:  "Unload a jrt-controller application from a jrt-controller controller.",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	addToFlags(cmd.PersistentFlags(), runOptions)
	jrtc.AddToFlags(cmd.PersistentFlags(), runOptions.controller)
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := errors.Join(
		opts.general.Parse(),
		opts.controller.Parse(),
		opts.parse(),
	); err != nil {
		return err
	}

	logger := opts.general.Logger

	g, ctx := errgroup.WithContext(cmd.Context())
	// TODO: Remove schemas for jrt-controller apps
	g.Go(func() error {
		client, err := jrtc.NewJrtcAPIClient(ctx, logger, opts.controller)
		if err != nil {
			return err
		}

		return App(logger, client, opts.appName)
	})

	return g.Wait()
}

// App unloads a jrt-controller app
func App(logger *logrus.Logger, client *jrtc.JrtcAPIClient, appName string) error {
	appStates, err := client.GetAppsWithResponse()
	if err != nil {
		return err
	}

	var appID int32 = -1
	for _, appState := range *appStates.JSON200 {
		if appState.Request.AppName == appName {
			appID = appState.Id

			dt, err := time.Parse(time.RFC3339, appState.StartTime)
			if err != nil {
				return err
			}

			logger.WithFields(logrus.Fields{
				"elapsedTime": time.Since(dt),
				"id":          appID,
				"startTime":   dt,
			}).Info("unloading app")
			break
		}
	}
	if appID != -1 {
		_, err = client.UnloadAppWithResponse(appID)
		if err != nil {
			return err
		}

		logger.Info("unloaded app")
	} else {
		logger.Warn("app not found")
	}
	return nil

}
