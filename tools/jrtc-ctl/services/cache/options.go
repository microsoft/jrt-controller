// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package cache

import (
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	defaultIP     = ""
	defaultPort   = 6379
	optionsPrefix = "redis"
	scheme        = "tcp"
)

// Options is the options for the cache client
type Options struct {
	ip      string
	port    uint16
	enabled bool
}

// AddToFlags adds the options to the flags
func AddToFlags(flags *pflag.FlagSet, opts *Options) {
	flags.StringVar(&opts.ip, optionsPrefix+"-ip", defaultIP, "IP address of the redis cache server")
	flags.Uint16Var(&opts.port, optionsPrefix+"-port", defaultPort, "port address of the redis cache server")
	flags.BoolVar(&opts.enabled, optionsPrefix+"-enabled", false, "whether redis cache is enabled")
}

// Parse parses the options
func (o *Options) Parse() error {
	if o.enabled {
		_, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", scheme, o.ip, o.port))
		if err != nil {
			return err
		}
	}

	return nil
}
