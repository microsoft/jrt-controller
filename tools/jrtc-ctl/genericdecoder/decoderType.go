// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package genericdecoder

import (
	"fmt"
	"jrtc-ctl/services/decoder"
	"strings"
)

type decoderType uint8

const (
	nilMethod decoderType = iota
	decodergrpc
	decoderhttp
)

var (
	allDecoderTypes              = []decoderType{decodergrpc, decoderhttp}
	allDecoderTypesStr           []string
	defaultDecoderType           = decodergrpc
	mapDecoderTypesToDefaultPort = map[decoderType]uint16{
		decodergrpc: decoder.DefaultControlPort,
		decoderhttp: decoder.DefaultControlGatewayPort,
	}
)

func init() {
	allDecoderTypesStr = make([]string, len(allDecoderTypes))
	for i, m := range allDecoderTypes {
		allDecoderTypesStr[i] = m.String()
	}
}

func (m decoderType) String() string {
	switch m {
	case decodergrpc:
		return "decodergrpc"
	case decoderhttp:
		return "decoderhttp"
	default:
		return "unknown"
	}
}

func (m decoderType) defaultPort() (p uint16) {
	p = mapDecoderTypesToDefaultPort[m]
	return p
}

func decoderTypeFromString(val string) (decoderType, error) {
	for _, m := range allDecoderTypes {
		if val == m.String() {
			return m, nil
		}
	}
	return nilMethod, fmt.Errorf("unrecognized decoder type %s, expected one of: %s", val, strings.Join(allDecoderTypesStr, ", "))
}
