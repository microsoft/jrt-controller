// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package load

import (
	"errors"
	"fmt"
	appLoad "jrtc-ctl/cmd/app/load"
	"jrtc-ctl/cmd/clicommon"
	jbpfLoad "jrtc-ctl/cmd/jbpf/load"
	"jrtc-ctl/common"
	"jrtc-ctl/genericdecoder"
	"jrtc-ctl/services/decoder"
	"jrtc-ctl/services/jbpf"
	jrtc "jrtc-ctl/services/jrt-controller"
	"os"

	"github.com/spf13/cobra"
	"golang.org/x/sync/errgroup"
)

type runOptions struct {
	general *common.GeneralOptions

	cli *clicommon.Options
}

// Command Load a jrt-controller application package
func Command(opts *common.GeneralOptions) *cobra.Command {
	runOptions := &runOptions{
		general: opts,

		cli: &clicommon.Options{},
	}
	cmd := &cobra.Command{
		Use:   "load",
		Short: "Load a jrt-controller application package",
		Long:  "Load a jrt-controller application package comprised of apps and codelet sets into a jrt-controller environment.",
		RunE: func(cmd *cobra.Command, _ []string) error {
			return run(cmd, runOptions)
		},
		SilenceUsage: true,
	}
	clicommon.AddToFlags(cmd.PersistentFlags(), runOptions.cli)
	return cmd
}

func run(cmd *cobra.Command, opts *runOptions) error {
	if err := opts.general.Parse(); err != nil {
		return err
	}

	cfg, err := opts.cli.Parse()
	if err != nil {
		return err
	}

	codeletCfgs := make(map[string]*clicommon.JBPFCLIConfig)
	for _, c := range cfg.JBPF.JBPFCodelets {
		jbpfCLIConfig, err := clicommon.NewJBPFCLIConfig(c.ConfigFile)
		if err != nil {
			return err
		}
		codeletCfgs[c.ConfigFile] = jbpfCLIConfig
	}

	logger := opts.general.Logger

	decoders := make([]*decoder.Client, 0, len(cfg.Decoders))
	for _, c := range cfg.Decoders {
		decoderOpts, err := genericdecoder.NewOptions(c.IP, c.Port, c.HTTPRelativePath, c.Typ)
		if err != nil {
			return err
		}

		switch do := decoderOpts.Inner.(type) {
		case *decoder.ClientOptions:
			dec, err := decoder.NewClient(cmd.Context(), logger, do)
			if err != nil {
				return err
			}
			decoders = append(decoders, dec)

			defer func(dec *decoder.Client) {
				if err := dec.Close(); err != nil {
					logger.WithError(err).Error("error closing decoder client connection")
				}
			}(dec)
		}
	}

	jbpfClients := make(map[uint8]*jbpf.Client, len(cfg.JBPF.Devices))
	for _, c := range cfg.JBPF.Devices {
		jbpfOpts, err := jbpf.NewOptions(c.IP, c.Port)
		if err != nil {
			return err
		}

		jbpfClients[c.ID], err = jbpf.NewClient(cmd.Context(), logger, jbpfOpts)
		if err != nil {
			return err
		}
	}

	jrtcClients := make(map[string]*jrtc.JrtcAPIClient)
	for _, c := range cfg.Apps {
		key := fmt.Sprintf("%s:%d", c.IP, c.Port)
		if _, ok := jrtcClients[key]; !ok {
			controllerOpts, err := jrtc.NewOptions(c.IP, c.Port)
			if err != nil {
				return err
			}

			jrtcClients[key], err = jrtc.NewJrtcAPIClient(cmd.Context(), logger, controllerOpts)
			if err != nil {
				return err
			}
		}
	}

	g, _ := errgroup.WithContext(cmd.Context())

	for _, app := range cfg.Apps {
		a := app
		// TODO: Send schemas for jrt-controller apps
		g.Go(func() error {
			key := fmt.Sprintf("%s:%d", a.IP, a.Port)
			client, ok := jrtcClients[key]
			if !ok {
				return fmt.Errorf(`missing "%s" jrt-controller Controller client`, key)
			}

			if a.AppParams == nil {
				a.AppParams = make(map[string]interface{})
			}
			// TODO: Fix the Python Loader to handle both single and multi-threaded apps
			if a.AppType == "python" || a.AppType == "python_single_app" {
				a.AppParams[a.AppType] = a.SharedLibraryPath
				a.SharedLibraryPath = os.ExpandEnv("${JRTC_PATH}/out/lib/libjrtc_pythonapp_loader.so")
				logger.Infof("Using python app loader: %s", a.SharedLibraryPath)
				logger.Infof("Python Type: %s", a.AppType)
				a.SharedLibraryCode, err = os.ReadFile(a.SharedLibraryPath)
				if err != nil {
					return err
				}
			}
			req, err := jrtc.NewJrtcAppLoadRequestFromBytes(a.SharedLibraryCode, a.SharedLibraryPath, a.Name, a.IOQSize, a.Deadline, a.Period, a.Runtime, a.AppType, &a.AppParams)
			if err != nil {
				return err
			}
			return appLoad.App(logger, client, req)
		})
	}

	for _, jbpfCodeletSet := range cfg.JBPF.JBPFCodelets {
		j := jbpfCodeletSet
		codeletCfg, ok := codeletCfgs[j.ConfigFile]
		if !ok {
			return errors.New("missing codelet config")
		}
		for _, currentDecoder := range decoders {
			dec := currentDecoder
			g.Go(func() error {
				return jbpfLoad.SchemasDecoder(dec, codeletCfg, cfg.Name)
			})
		}
	}

	if err := g.Wait(); err != nil {
		return err
	}

	for _, jbpfCodeletSet := range cfg.JBPF.JBPFCodelets {
		j := jbpfCodeletSet
		codeletCfg := codeletCfgs[j.ConfigFile]
		g.Go(func() error {
			return jbpfLoad.CodeletSet(jbpfClients[j.Device], codeletCfg, cfg.Name, j.Device)
		})
	}

	return g.Wait()
}
