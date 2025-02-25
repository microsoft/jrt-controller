// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package unload

import (
	"jrtc-ctl/cmd/clicommon"
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/genericdecoder"
	"jrtc-ctl/services/decoder"
	"jrtc-ctl/services/jbpf"
	"errors"
	"fmt"

	"github.com/spf13/cobra"
	"github.com/spf13/pflag"
	"golang.org/x/sync/errgroup"
)

const (
	defaultJrtcAppName = "jrtc_app"
	maxDeviceID          = 1<<7 - 1
	streamPathFormat     = "%s://jbpf_agent/%s/%s"
)

type runOptions struct {
	general *common.GeneralOptions

	cli     *clicommon.JBPFOptions
	decoder *genericdecoder.Options
	jbpf    *jbpf.Options

	jrtcAppName  string
	jrtcDeviceID uint8
}

func addToFlags(flags *pflag.FlagSet, opts *runOptions) {
	flags.StringVar(&opts.jrtcAppName, "app-name", defaultJrtcAppName, "app name inside jrt-controller")
	flags.Uint8Var(&opts.jrtcDeviceID, "device-id", 1, "device ID inside jrt-controller, must be unique")
}

func (o *runOptions) parse() error {
	if o.jrtcDeviceID > maxDeviceID {
		return fmt.Errorf("device ID must be less or equal to %d", maxDeviceID)
	}

	return nil
}

// Command Unload a jbpf codelet
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		general: opts,
		cli:     &clicommon.JBPFOptions{},
		decoder: &genericdecoder.Options{},
		jbpf:    &jbpf.Options{},
	}
	cmd := &cobra.Command{
		Use:   "unload",
		Short: "Unload a jbpf codelet",
		Long:  "Unload a jbpf codelet from a JBPF device.",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	addToFlags(cmd.PersistentFlags(), runOptions)
	clicommon.AddJBPFOptionsToFlags(cmd.PersistentFlags(), runOptions.cli)
	genericdecoder.AddToFlags(cmd.PersistentFlags(), runOptions.decoder)
	jbpf.AddToFlags(cmd.PersistentFlags(), runOptions.jbpf)
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := errors.Join(
		opts.general.Parse(),
		opts.jbpf.Parse(),
		opts.decoder.Parse(),
		opts.parse(),
	); err != nil {
		return err
	}

	cfg, err := opts.cli.Parse()
	if err != nil {
		return err
	}

	logger := opts.general.Logger

	g, ctx := errgroup.WithContext(cmd.Context())

	g.Go(func() error {
		client, err := jbpf.NewClient(ctx, logger, opts.jbpf)
		if err != nil {
			return err
		}
		return client.UnloadCodeletSet(cfg.ID)
	})
	appErr := g.Wait()

	g.Go(func() error {
		if opts.decoder.Inner == nil {
			return nil
		}
		switch do := opts.decoder.Inner.(type) {
		case *decoder.ClientOptions:
			dec, err := decoder.NewClient(ctx, logger, do)
			if err != nil {
				return err
			}
			if dec == nil {
				return nil
			}
			defer func() {
				if err := dec.Close(); err != nil {
					logger.WithError(err).Errorf("error closing decoder client")
				}
			}()

			return SchemasDecoder(dec, cfg, opts.jrtcAppName)
		}

		return fmt.Errorf(`unrecognized decoder type "%T"`, opts.decoder.Inner)
	})

	return errors.Join(appErr, g.Wait())
}

// SchemasDecoder unloads jbpf schemas from a local decoder
func SchemasDecoder(dec *decoder.Client, cfg *clicommon.JBPFCLIConfig, appName string) error {
	streamIDBs := make([][]byte, 0)

	for _, c := range cfg.Codelets {
		for _, io := range c.OutIOChannel {
			streamPath := fmt.Sprintf(streamPathFormat, appName, cfg.ID, c.Name)
			forwardDestination, err := jrtcbindings.DestinationFromString(io.ForwardDestination)
			if err != nil {
				return err
			}
			streamUUID, err := jrtcbindings.GenerateStreamID(
				forwardDestination,
				jrtcbindings.DeviceIDAny,
				&streamPath,
				&io.Name,
			)
			if err != nil {
				return err
			}
			streamIDB, err := streamUUID.MarshalBinary()
			if err != nil {
				return err
			}
			streamIDBs = append(streamIDBs, streamIDB)
		}
	}

	return dec.Unload(streamIDBs)
}
