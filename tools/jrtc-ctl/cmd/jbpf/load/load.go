// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package load

import (
	"jrtc-ctl/cmd/clicommon"
	"jrtc-ctl/common"
	"jrtc-ctl/jrtcbindings"
	"jrtc-ctl/genericdecoder"
	"jrtc-ctl/services/decoder"
	"jrtc-ctl/services/jbpf"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strings"

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

// Command Load a jbpf codelet
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		general: opts,

		cli:     &clicommon.JBPFOptions{},
		decoder: &genericdecoder.Options{},
		jbpf:    &jbpf.Options{},
	}
	cmd := &cobra.Command{
		Use:   "load",
		Short: "Load a jbpf codelet",
		Long:  "Load a jbpf codelet to a JBPF device.",
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
		opts.decoder.Parse(),
		opts.general.Parse(),
		opts.jbpf.Parse(),
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
	if err := g.Wait(); err != nil {
		return err
	}

	g.Go(func() error {
		client, err := jbpf.NewClient(ctx, logger, opts.jbpf)
		if err != nil {
			return err
		}
		return CodeletSet(client, cfg, opts.jrtcAppName, opts.jrtcDeviceID)
	})

	return g.Wait()
}

// SchemasDecoder loads jbpf schemas to local decoder
func SchemasDecoder(dec *decoder.Client, cfg *clicommon.JBPFCLIConfig, appName string) error {
	schemas := make(map[string]*decoder.LoadRequest)

	for _, c := range cfg.Codelets {
		for _, io := range c.OutIOChannel {
			if io.Serde == nil || io.Serde.Protobuf == nil {
				continue
			}

			protoName := strings.TrimSuffix(filepath.Base(io.Serde.Protobuf.PackagePath), ".pb")

			if _, ok := schemas[protoName]; !ok {
				proto, err := os.ReadFile(io.Serde.Protobuf.PackagePath)
				if err != nil {
					return err
				}

				schemas[protoName] = &decoder.LoadRequest{
					CompiledProto: proto,
					Streams:       make(map[string][]byte),
				}
			}

			streamPath := fmt.Sprintf(streamPathFormat, appName, cfg.ID, c.Name)
			streamUUID, err := jrtcbindings.GenerateStreamID(
				jrtcbindings.DestinationAny,
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

			schemas[protoName].Streams[io.Serde.Protobuf.MessageName] = streamIDB
		}
	}

	return dec.Load(schemas)
}

// CodeletSet loads a jbpf codelet set
func CodeletSet(client *jbpf.Client, cfg *clicommon.JBPFCLIConfig, appName string, deviceID uint8) error {
	req := make([]*jbpf.LoadCodeletSetRequestDescriptors, 0, len(cfg.Codelets))
	for _, c := range cfg.Codelets {
		inputMaps := make([]*jbpf.LoadCodeletSetRequestIOMap, 0, len(c.InIOChannel))
		for _, io := range c.InIOChannel {
			streamPath := fmt.Sprintf(streamPathFormat, appName, cfg.ID, c.Name)
			streamUUID, err := jrtcbindings.GenerateStreamID(
				jrtcbindings.DestinationNone,
				deviceID,
				&streamPath,
				&io.Name,
			)
			if err != nil {
				return err
			}

			m := &jbpf.LoadCodeletSetRequestIOMap{
				Name:     io.Name,
				StreamID: strings.ReplaceAll(streamUUID.String(), "-", ""),
			}
			if io.Serde != nil && len(io.Serde.FilePath) > 0 {
				m.Serde = &jbpf.LoadCodeletSetRequestSerde{
					FilePath: io.Serde.FilePath,
				}
			}

			inputMaps = append(inputMaps, m)
		}

		outputMaps := make([]*jbpf.LoadCodeletSetRequestIOMap, 0, len(c.OutIOChannel))
		for _, io := range c.OutIOChannel {
			streamPath := fmt.Sprintf(streamPathFormat, appName, cfg.ID, c.Name)
			forwardDestination, err := jrtcbindings.DestinationFromString(io.ForwardDestination)
			if err != nil {
				return err
			}
			streamUUID, err := jrtcbindings.GenerateStreamID(
				forwardDestination,
				deviceID,
				&streamPath,
				&io.Name,
			)
			if err != nil {
				return err
			}

			m := &jbpf.LoadCodeletSetRequestIOMap{
				Name:     io.Name,
				StreamID: strings.ReplaceAll(streamUUID.String(), "-", ""),
			}
			if io.Serde != nil && len(io.Serde.FilePath) > 0 {
				m.Serde = &jbpf.LoadCodeletSetRequestSerde{
					FilePath: io.Serde.FilePath,
				}
			}

			outputMaps = append(outputMaps, m)
		}

		linkedMaps := make([]*jbpf.LoadCodeletSetRequestLinkedMap, 0, len(c.LinkedMaps))
		for _, l := range c.LinkedMaps {
			linkedMaps = append(linkedMaps, &jbpf.LoadCodeletSetRequestLinkedMap{
				LinkedCodeletName: l.LinkedCodeletName,
				LinkedMapName:     l.LinkedMapName,
				MapName:           l.MapName,
			})
		}

		desc := &jbpf.LoadCodeletSetRequestDescriptors{
			CodeletName:      c.Name,
			CodeletPath:      c.CodeletPath,
			HookName:         c.HookName,
			InIOChannel:      inputMaps,
			LinkedMaps:       linkedMaps,
			OutIOChannel:     outputMaps,
			Priority:         c.Priority,
			RuntimeThreshold: c.RuntimeThreshold,
		}
		req = append(req, desc)
	}

	return client.LoadCodeletSet(&jbpf.LoadCodeletSetRequest{
		CodeletDescriptors: req,
		CodeletSetID:       cfg.ID,
	})
}
