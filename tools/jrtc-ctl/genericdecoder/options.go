// Copyright (c) Microsoft Corporation. All rights reserved.

package genericdecoder

import (
	"fmt"
	"jrtc-ctl/services/decoder"
	"strings"

	"github.com/spf13/pflag"
)

const (
	optionsPrefix = "decoder"
)

// Options is the options for any decoder client
type Options struct {
	enabled          bool
	httpRelativePath string
	ip               string
	port             uint16
	typ              string
	// Inner is the concrete decoder config, either *decoder.ClientOptions or *streamreader.Options
	Inner interface{}
}

// NewOptions creates a new decoder options and converts to concrete options
func NewOptions(ip string, port uint16, httpRelativePath string, typ string) (*Options, error) {
	opts := &Options{
		enabled:          true,
		httpRelativePath: httpRelativePath,
		ip:               ip,
		port:             port,
		typ:              typ,
	}

	return opts, opts.Parse()
}

// AddToFlags adds generic decoder options to flags
func AddToFlags(flags *pflag.FlagSet, opts *Options) {
	flags.BoolVar(&opts.enabled, optionsPrefix+"-enable", false, "whether decoder is enabled")
	flags.StringVar(&opts.httpRelativePath, optionsPrefix+"-http-relative-path", "", "http relative path for the decoder, i.e. http://<ip>:<port><relative-path>")
	flags.StringVar(&opts.ip, optionsPrefix+"-ip", "localhost", "IP address of the decoder")
	flags.StringVar(&opts.typ, optionsPrefix+"-type", defaultDecoderType.String(), fmt.Sprintf("type of decoder, expected to be \"%s\"", strings.Join(allDecoderTypesStr, "\", \"")))
	flags.Uint16Var(&opts.port, optionsPrefix+"-port", 0, fmt.Sprintf("port address of the decoder, defaults based on %s-type (%s = %d, %s = %d)", optionsPrefix, decodergrpc, decodergrpc.defaultPort(), decoderhttp, decoderhttp.defaultPort()))
}

// Parse converts to concrete decoder config
func (o *Options) Parse() error {
	if !o.enabled {
		return nil
	}

	t, err := decoderTypeFromString(o.typ)
	if err != nil {
		return err
	}

	switch t {
	case decodergrpc:
		cfg, err := decoder.NewClientOptions(o.ip, o.port, "", false)
		if err != nil {
			return err
		}
		if err := cfg.Parse(); err != nil {
			return err
		}
		o.Inner = cfg
		return nil
	case decoderhttp:
		cfg, err := decoder.NewClientOptions(o.ip, o.port, o.httpRelativePath, true)
		if err != nil {
			return err
		}
		if err := cfg.Parse(); err != nil {
			return err
		}
		o.Inner = cfg
		return nil
	}

	return fmt.Errorf(`unrecognized decoder type "%s"`, o.typ)
}
