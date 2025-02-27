// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jrtc

import (
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	jrtcPrefix = "jrtc"
	scheme       = "http"
	defaultIP    = "localhost"
	defaultPort  = 3001
)

// Options for jrt-controller
type Options struct {
	_ip   string
	_port uint16
	addr  string
}

// NewOptions creates a new options instance and parses the options
func NewOptions(ip string, port uint16) (*Options, error) {
	o := &Options{
		_ip:   ip,
		_port: port,
	}
	if err := o.Parse(); err != nil {
		return nil, err
	}
	return o, nil
}

// AddToFlags adds flags to flagset
func AddToFlags(flags *pflag.FlagSet, opts *Options) {
	flags.StringVar(&opts._ip, jrtcPrefix+"-ip", defaultIP, "IP address of the jrt-controller client control interface")
	flags.Uint16Var(&opts._port, jrtcPrefix+"-port", defaultPort, "port of the jrt-controller client control interface")
}

// Parse validates options
func (o *Options) Parse() error {
	u, err := url.ParseRequestURI(fmt.Sprintf("%s://%s:%d", scheme, o._ip, o._port))
	if err != nil {
		return err
	}
	o.addr = fmt.Sprintf("%s://%s", u.Scheme, u.Host)

	return nil
}
