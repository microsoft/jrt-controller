// Copyright (c) Microsoft Corporation. All rights reserved.

package clicommon

import (
	"fmt"
	"os"

	"github.com/spf13/pflag"
)

// Options for jrtc-ctl
type Options struct {
	config []string
}

// AddToFlags adds codelet set options to flags
func AddToFlags(flags *pflag.FlagSet, opts *Options) {
	flags.StringArrayVarP(&opts.config, "config", "c", []string{}, "the path(s) where to look for the deployment config")
}

// Parse validates codelet set options and returns the parsed cli config
func (o *Options) Parse() (*CLIConfig, error) {
	ret, err := FromYamls(o.config...)
	if err != nil {
		return nil, err
	}
	for _, a := range ret.Apps {
		fi, err := os.Stat(a.SharedLibraryPath)
		if err != nil {
			return nil, err
		} else if fi.IsDir() {
			return nil, fmt.Errorf("expected %s to be a file", a.SharedLibraryPath)
		}

		a.SharedLibraryCode, err = os.ReadFile(a.SharedLibraryPath)
		if err != nil {
			return nil, err
		}
	}

	return ret, nil
}
