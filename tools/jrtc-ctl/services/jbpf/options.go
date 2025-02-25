// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jbpf

import (
	"fmt"
	"net/url"

	"github.com/spf13/pflag"
)

const (
	defaultIP     = "127.0.0.1"
	defaultPort   = uint16(30590)
	optionsPrefix = "jbpf"
)

// Options for jbpf
type Options struct {
	_ip   string
	_port uint16
	addr  *url.URL
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
	if opts == nil {
		return
	}

	flags.StringVar(&opts._ip, optionsPrefix+"-ip", defaultIP, "IP address of the JBPF client control REST interface")
	flags.Uint16Var(&opts._port, optionsPrefix+"-port", defaultPort, "port of the JBPF client control REST interface")
}

// Parse validates options
func (o *Options) Parse() error {
	url, err := url.Parse(fmt.Sprintf("http://%s:%d", o._ip, o._port))
	if err != nil {
		return err
	}
	o.addr = url

	return nil
}

// GetHost returns host from options
func (o *Options) GetHost() string {
	return o.addr.Host
}
