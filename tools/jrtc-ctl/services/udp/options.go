// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package udp

import (
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	dataPrefix            = "json-data"
	dataScheme            = "udp"
	defaultDataBufferSize = 1<<16 - 1
	defaultDataIP         = ""
	defaultDataPort       = uint16(20786)
)

// ServerOptions is the options for the decoder server
type ServerOptions struct {
	dataBufferSize uint16
	dataEnabled    bool
	dataIP         string
	dataPort       uint16
}

// EmptyServerOptions returns an empty server options
func EmptyServerOptions() *ServerOptions {
	return &ServerOptions{}
}

// AddServerOptionsToFlags adds the server options to the flags
func AddServerOptionsToFlags(flags *pflag.FlagSet, opts *ServerOptions, autoEnabled bool) {
	if autoEnabled {
		opts.dataEnabled = true
	} else {
		flags.BoolVar(&opts.dataEnabled, dataPrefix+"-enabled", false, "whether json data processing is enabled")
	}
	flags.StringVar(&opts.dataIP, dataPrefix+"-ip", defaultDataIP, "IP address of the json data UDP server")
	flags.Uint16Var(&opts.dataBufferSize, dataPrefix+"-buffer", defaultDataBufferSize, "buffer size for the json data UDP server")
	flags.Uint16Var(&opts.dataPort, dataPrefix+"-port", defaultDataPort, "port address of the json data UDP server")
}

// Parse parses the server options
func (o *ServerOptions) Parse() error {
	_, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", dataScheme, o.dataIP, o.dataPort))
	if err != nil {
		return err
	}

	return nil
}
