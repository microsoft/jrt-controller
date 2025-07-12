// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package load

import (
	"errors"
	"fmt"
	"jrtc-ctl/common"
	jrtc "jrtc-ctl/services/jrt-controller"
	"time"

	"github.com/sirupsen/logrus"
	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
	"golang.org/x/sync/errgroup"
)

const (
	defaultDeadline = time.Second * 0
	defaultIOQSize  = 1000
	defaultPeriod   = time.Second * 0
	defaultRuntime  = time.Second * 0
)

type runOptions struct {
	general    *common.GeneralOptions
	controller *jrtc.Options

	_deadline         string
	_period           string
	_runtime          string
	appName           string
	deadline          time.Duration
	ioqSize           int32
	period            time.Duration
	runtime           time.Duration
	sharedLibraryPath string
	appType           string
	appParams         map[string]interface{}
	deviceMapping     map[string]interface{}
	appModules        []string
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.Int32Var(&opts.ioqSize, "ioq-size", defaultIOQSize, "the message queue size of jrt-controller router for shipping data to the app")
	flags.StringVar(&opts._deadline, "deadline", fmt.Sprint(defaultDeadline), "the deadline of the thread in microseconds. Set to 0 to disable deadline scheduling")
	flags.StringVar(&opts._period, "period", fmt.Sprint(defaultPeriod), "the period of the thread")
	flags.StringVar(&opts._runtime, "runtime", fmt.Sprint(defaultRuntime), "the runtime quota of the thread")
	flags.StringVar(&opts.appName, "app-name", "", "app name inside jrt-controller")
	flags.StringVar(&opts.sharedLibraryPath, "app-path", "", "the shared library of the jrt-controller app")
	flags.StringVar(&opts.appType, "app-type", "", "the type of the app")
}

func (o *runOptions) parse() error {
	dur, err := time.ParseDuration(o._deadline)
	if err != nil {
		return err
	}
	o.deadline = dur

	dur, err = time.ParseDuration(o._period)
	if err != nil {
		return err
	}
	o.period = dur

	dur, err = time.ParseDuration(o._runtime)
	if err != nil {
		return err
	}
	o.runtime = dur

	if len(o.appName) == 0 {
		return fmt.Errorf("--app-name is required")
	}

	return nil
}

// Command Load a jrt-controller application
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		general:    opts,
		controller: &jrtc.Options{},
	}
	cmd := &cobra.Command{
		Use:   "load",
		Short: "Load a jrt-controller application",
		Long:  "Load a jrt-controller application to a jrt-controller controller.",
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
	// TODO: Send schemas for jrt-controller apps
	g.Go(func() error {
		client, err := jrtc.NewJrtcAPIClient(ctx, logger, opts.controller)
		if err != nil {
			return err
		}

		req, err := jrtc.NewJrtcAppLoadRequest(opts.sharedLibraryPath, opts.appName, opts.ioqSize, opts.deadline, opts.period, opts.runtime, opts.appType, &opts.appModules, &opts.appParams, &opts.deviceMapping)
		if err != nil {
			return err
		}

		return App(logger, client, req)
	})

	return g.Wait()
}

// App loads a jrt-controller app
func App(logger *logrus.Logger, client *jrtc.JrtcAPIClient, req *jrtc.JrtcAppLoadRequest) error {
	out, err := client.LoadAppWithResponse(*req)
	if err != nil {
		return err
	}

	dt, err := time.Parse(time.RFC3339, out.JSON200.StartTime)
	if err != nil {
		return err
	}

	logger.WithFields(logrus.Fields{
		"id":        out.JSON200.Id,
		"startTime": dt,
	}).Info("loaded app")

	return nil
}
