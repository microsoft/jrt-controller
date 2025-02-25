// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package decoder

import "github.com/spf13/pflag"

// ClientOptions is the options for the decoder client
type ClientOptions struct {
	control          *controlOptions
	enabled          bool
	httpRelativePath string
	useGateway       bool
}

// EmptyClientOptions returns an empty client options
func EmptyClientOptions() *ClientOptions {
	return &ClientOptions{control: &controlOptions{}}
}

// NewClientOptions creates a new server options and parses the options
func NewClientOptions(ip string, port uint16, httpRelativePath string, useGateway bool) (*ClientOptions, error) {
	opts := &ClientOptions{
		control: &controlOptions{
			ip:   ip,
			port: port,
		},
		enabled:          true,
		httpRelativePath: httpRelativePath,
		useGateway:       useGateway,
	}

	return opts, opts.Parse()
}

// AddClientOptionsToFlags adds the client options to the flags
func AddClientOptionsToFlags(flags *pflag.FlagSet, opts *ClientOptions) {
	addControlOptionsToFlags(flags, opts.control)

	flags.BoolVar(&opts.enabled, controlPrefix+"-enable", false, "whether decoder is enabled")
	flags.BoolVar(&opts.useGateway, controlPrefix+"-gateway", false, "whether to use http gateway rather than gRPC")
	flags.StringVar(&opts.httpRelativePath, controlPrefix+"-http-relative-path", "", "http relative path for the decoder, i.e. http://<ip>:<port><relative-path>")
}

// Parse parses the client options
func (o *ClientOptions) Parse() error {
	return o.control.parse()
}
