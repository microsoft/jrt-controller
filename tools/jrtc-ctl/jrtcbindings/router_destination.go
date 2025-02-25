// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jrtcbindings

// #include <stdlib.h>
// #include "jrtc_router_stream_id.h"
import "C"
import "fmt"

// Destination is an enum type for the destination of the stream
type Destination uint8

const (
	// DestinationAny will be sent to any destination
	DestinationAny Destination = C.JRTC_ROUTER_DEST_ANY
	// DestinationNone will not be sent out from the jrt-controller
	DestinationNone Destination = C.JRTC_ROUTER_DEST_NONE
	// DestinationUDP will be sent to UDP port
	DestinationUDP Destination = C.JRTC_ROUTER_DEST_UDP
)

const (
	// DefaultDestination is the default destination for the stream
	DefaultDestination = DestinationNone
)

var (
	destinationFromString = map[string]Destination{
		"DestinationAny":  DestinationAny,
		"DestinationNone": DestinationNone,
		"DestinationUDP":  DestinationUDP,
	}
	destinationFromStringAlt = map[string]Destination{
		"JRTC_ROUTER_DEST_ANY":   DestinationAny,
		"JRTC_ROUTER_DEST_NONE":  DestinationNone,
		"JRTC_ROUTER_DEST_UDP":   DestinationUDP,
	}
	destinationToString map[Destination]string
	// AllDestinationsStr is a list of all jrt-controller router destinations as strings
	AllDestinationsStr []string
)

func init() {
	destinationToString = make(map[Destination]string, len(destinationFromString))
	AllDestinationsStr = make([]string, 0, len(destinationFromString))
	for strVal, routerVal := range destinationFromString {
		destinationToString[routerVal] = strVal
		AllDestinationsStr = append(AllDestinationsStr, strVal)
	}
}

// DestinationFromString converts a string to a Destination
func DestinationFromString(val string) (Destination, error) {
	dest, ok := destinationFromString[val]
	if ok {
		return dest, nil
	}
	dest, ok = destinationFromStringAlt[val]
	if ok {
		return dest, nil
	}
	return DestinationNone, fmt.Errorf(`unrecognized jrt-controller router destination: "%s"`, val)
}

func (d Destination) String() string {
	return destinationToString[d]
}
