// Copyright (c) Microsoft Corporation. All rights reserved.

package jrtcbindings

// #include <stdlib.h>
// #include "jrtc_router_stream_id.h"
import "C"
import (
	"fmt"
	"unsafe"
)

// StreamID is a unique identifier for a stream inside the jrt-controller
type StreamID [16]byte

func (s StreamID) asCStruct() *C.jrtc_router_stream_id_t {
	out := C.jrtc_router_stream_id_t{}
	for i := range s {
		out.id[i] = C.char(s[i])
	}
	return &out
}

func newStreamID(internal *C.jrtc_router_stream_id_t) (StreamID, error) {
	var s StreamID
	data := C.GoBytes(unsafe.Pointer(&internal.id[0]), 16)
	copy(s[:], data)
	if err := s.UnmarshalBinary(data); err != nil {
		return StreamIDNil, err
	}
	return s, nil
}

// GenerateStreamID generates a stream ID for a given forward destination, device ID, stream path, and stream name
func GenerateStreamID(forwardDestination Destination, deviceID uint8, streamPath, streamName *string) (StreamID, error) {
	streamID := &C.jrtc_router_stream_id_t{}
	streamPathCharArr := (*C.char)(nil)
	if streamPath != nil {
		streamPathCharArr = C.CString(*streamPath)
		defer C.free(unsafe.Pointer(streamPathCharArr))
	}
	streamNameCharArr := (*C.char)(nil)
	if streamName != nil {
		streamNameCharArr = C.CString(*streamName)
		defer C.free(unsafe.Pointer(streamNameCharArr))
	}
	if res := C.jrtc_router_generate_stream_id(streamID, C.int(forwardDestination), C.int(deviceID), streamPathCharArr, streamNameCharArr); res != C.int(1) {
		return StreamIDNil, fmt.Errorf("Failed to generate stream ID")
	}
	return newStreamID(streamID)
}

// Matches checks if a stream ID matches a given stream ID request
func (s StreamID) Matches(streamIDReq StreamID) (bool, error) {
	internal1 := s.asCStruct()
	internal2 := streamIDReq.asCStruct()
	isMatch := C.jrtc_router_stream_id_matches_req(internal1, internal2)
	return bool(isMatch), nil
}

// GetVersion returns the version of the stream ID
func (s *StreamID) GetVersion() byte {
	// XXXXXX00 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	return (s[0] >> 2) & 0x3F
}

// GetForwardingDestination returns the version of the forwarding destination
func (s *StreamID) GetForwardingDestination() byte {
	// 000000XX XXXXX000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	return ((s[0] & 0x03) << 5) | ((s[1] >> 3) & 0x1F)
}

// GetDeviceID returns the version of the device ID
func (s *StreamID) GetDeviceID() byte {
	// 00000000 00000XXX XXXX0000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
	return ((s[1] & 0x07) << 4) | ((s[2] >> 4) & 0x0F)
}

// GetStreamPath returns the version of the stream path
func (s *StreamID) GetStreamPath() []byte {
	// 00000000 00000000 0000XXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XX000000 00000000 00000000 00000000 00000000 00000000 00000000
	return []byte{(s[2]&0x0F)<<2 | s[3]>>6, s[3]<<2 | s[4]>>6, s[4]<<2 | s[5]>>6, s[5]<<2 | s[6]>>6, s[6]<<2 | s[7]>>6, s[7]<<2 | s[8]>>6, s[8]<<2 | s[9]>>6}
}

// GetStreamName returns the version of the stream name
func (s *StreamID) GetStreamName() []byte {
	// 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00XXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX
	return []byte{s[9] & 0x3F, s[10], s[11], s[12], s[13], s[14], s[15]}
}

// StreamIDFormatConfig is a configuration for formatting a stream ID
type StreamIDFormatConfig struct {
	ClearVersion               bool
	ClearForwardingDestination bool
	ClearDeviceID              bool
	ClearStreamPath            bool
	ClearStreamName            bool
}

// Format formats the ID
func (s *StreamID) Format(req *StreamIDFormatConfig) {
	if req.ClearVersion {
		s[0] &= 0x03
	}
	if req.ClearForwardingDestination {
		s[0] &= 0xFC
		s[1] &= 0x07
	}
	if req.ClearDeviceID {
		s[1] &= 0xF8
		s[2] &= 0x0F
	}
	if req.ClearStreamPath {
		s[2] &= 0xF0
		s[3] = 0
		s[4] = 0
		s[5] = 0
		s[6] = 0
		s[7] = 0
		s[8] &= 0
		s[9] &= 0x3F
	}
	if req.ClearStreamName {
		s[9] &= 0xC0
		s[10] = 0
		s[11] = 0
		s[12] = 0
		s[13] = 0
		s[14] = 0
		s[15] = 0
	}
}
