// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package decoder

import (
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	// DefaultControlPort is the default used for the local decoder server
	DefaultControlPort = uint16(20789)
	// DefaultControlGatewayPort is the default used for the local decoder server gateway
	DefaultControlGatewayPort = uint16(20787)

	controlGatewayScheme         = "http"
	controlPrefix                = "decoder-control"
	controlScheme                = "tcp"
	defaultControlGatewayEnabled = false
	defaultControlGatewayIP      = ""
	defaultControlIP             = ""
)

type controlOptions struct {
	ip   string
	port uint16
}

func addControlOptionsToFlags(flags *pflag.FlagSet, opts *controlOptions) {
	flags.StringVar(&opts.ip, controlPrefix+"-ip", defaultControlIP, "IP address of the control gRPC/HTTP server")
	flags.Uint16Var(&opts.port, controlPrefix+"-port", DefaultControlPort, "port address of the control gRPC/HTTP server")
}

func (o *controlOptions) parse() error {
	_, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", controlScheme, o.ip, o.port))
	if err != nil {
		return err
	}

	return nil
}

type gatewayOptions struct {
	enabled bool
	ip      string
	port    uint16
}

func addGatewayOptionsToFlags(flags *pflag.FlagSet, opts *gatewayOptions) {
	flags.BoolVar(&opts.enabled, controlPrefix+"-gateway-enabled", defaultControlGatewayEnabled, "whether control REST gateway is enabled")
	flags.StringVar(&opts.ip, controlPrefix+"-gateway-ip", defaultControlGatewayIP, "IP address of the gateway REST server")
	flags.Uint16Var(&opts.port, controlPrefix+"-gateway-port", DefaultControlGatewayPort, "port address of the gateway REST server")
}

func (o *gatewayOptions) parse() error {
	if o.enabled {
		_, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", controlGatewayScheme, o.ip, o.port))
		if err != nil {
			return err
		}
	}

	return nil
}
