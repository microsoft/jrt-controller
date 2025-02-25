// Copyright (c) Microsoft Corporation. All rights reserved.

package clicommon

import (
	"os"
	"path/filepath"

	"github.com/spf13/pflag"
)

// JBPFOptions for jbpf
type JBPFOptions struct {
	config string
}

// AddJBPFOptionsToFlags adds codelet set options to flags
func AddJBPFOptionsToFlags(flags *pflag.FlagSet, opts *JBPFOptions) {
	flags.StringVar(&opts.config, "config", "", "the yaml file to load. Can be relative or absolute path from working directory and can include environment variables")
}

// NewJBPFCLIConfig creates a new CLI Config
func NewJBPFCLIConfig(config string) (*JBPFCLIConfig, error) {
	o := &JBPFOptions{
		config: config,
	}
	return o.Parse()
}

// Parse validates codelet set options and returns the parsed cli config
func (o *JBPFOptions) Parse() (*JBPFCLIConfig, error) {
	configFile, err := filepath.Abs(os.ExpandEnv(o.config))
	if err != nil {
		return nil, err
	}
	parsedConfig, err := JBPFConfigFromYaml(configFile)
	if err != nil {
		return nil, err
	}

	return parsedConfig, nil
}
