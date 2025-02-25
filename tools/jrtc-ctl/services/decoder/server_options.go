// Copyright (c) Microsoft Corporation. All rights reserved.

package decoder

import (
	"errors"
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	dataPrefix            = "decoder-data"
	dataScheme            = "udp"
	defaultDataBufferSize = 1<<16 - 1
	defaultDataIP         = ""
	defaultDataPort       = uint16(20788)
)

// ServerOptions is the options for the decoder server
type ServerOptions struct {
	control *controlOptions
	gateway *gatewayOptions

	dataBufferSize uint16
	dataEnabled    bool
	dataIP         string
	dataPort       uint16
}

// EmptyServerOptions returns an empty server options
func EmptyServerOptions() *ServerOptions {
	return &ServerOptions{control: &controlOptions{}, gateway: &gatewayOptions{}}
}

// AddServerOptionsToFlags adds the server options to the flags
func AddServerOptionsToFlags(flags *pflag.FlagSet, opts *ServerOptions) {
	addControlOptionsToFlags(flags, opts.control)
	addGatewayOptionsToFlags(flags, opts.gateway)

	flags.BoolVar(&opts.dataEnabled, dataPrefix+"-enabled", false, "whether data processing is enabled")
	flags.StringVar(&opts.dataIP, dataPrefix+"-ip", defaultDataIP, "IP address of the data UDP server")
	flags.Uint16Var(&opts.dataBufferSize, dataPrefix+"-buffer", defaultDataBufferSize, "buffer size for the data UDP server")
	flags.Uint16Var(&opts.dataPort, dataPrefix+"-port", defaultDataPort, "port address of the data UDP server")
}

// Parse parses the server options
func (o *ServerOptions) Parse() error {
	if err := errors.Join(o.control.parse(), o.gateway.parse()); err != nil {
		return err
	}

	_, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", dataScheme, o.dataIP, o.dataPort))
	if err != nil {
		return err
	}

	return nil
}
