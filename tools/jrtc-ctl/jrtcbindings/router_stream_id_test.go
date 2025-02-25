// Copyright (c) Microsoft Corporation. All rights reserved.

package jrtcbindings

import (
	"encoding/hex"
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

type generateStreamIDTestConfig struct {
	forwardDestination Destination
	deviceID           uint8
	streamPath         string
	streamName         string
	expectedID         string
	expectedStreamPath string
	expectedStreamName string
}

const (
	testDestination = DestinationUDP
	testDeviceID    = 1
	testStreamPath  = "cpudist_percore/percpu_hist_map_t"
	testStreamName  = "placeholder"

	altDestination = DestinationNone
	altDeviceID    = 2
	altStreamPath  = "cpudist_percore/percpu_hist_map_t2"
	altStreamName  = "placeholder2"
)

func TestGenerateStreamID(t *testing.T) {
	testCases := []generateStreamIDTestConfig{
		{testDestination, testDeviceID, testStreamPath, testStreamName, "00101e30-97b1-454b-fcaf-cac30c2235d9", "38c25ec5152ff2", "2fcac30c2235d9"},
		{altDestination, testDeviceID, testStreamPath, testStreamName, "00081e30-97b1-454b-fcaf-cac30c2235d9", "38c25ec5152ff2", "2fcac30c2235d9"},
		{testDestination, altDeviceID, testStreamPath, testStreamName, "00102e30-97b1-454b-fcaf-cac30c2235d9", "38c25ec5152ff2", "2fcac30c2235d9"},
		{testDestination, testDeviceID, altStreamPath, testStreamName, "00101fd3-9066-db03-2f2f-cac30c2235d9", "3f4e419b6c0cbc", "2fcac30c2235d9"},
		{testDestination, testDeviceID, testStreamPath, altStreamName, "00101e30-97b1-454b-fc86-fa8674e722c2", "38c25ec5152ff2", "06fa8674e722c2"},
	}

	for idx, testCase := range testCases {
		t.Run(fmt.Sprintf("GenerateStreamID_%d", idx), func(t *testing.T) {
			id, err := GenerateStreamID(testCase.forwardDestination, testCase.deviceID, &testCase.streamPath, &testCase.streamName)
			require.Nil(t, err)
			require.Equal(t, testCase.expectedID, id.String())
			require.Equal(t, uint8(0), id.GetVersion())
			require.Equal(t, testCase.deviceID, id.GetDeviceID())
			require.Equal(t, uint8(testCase.forwardDestination), id.GetForwardingDestination())
			streamPath := id.GetStreamPath()
			streamPathStr := make([]byte, hex.EncodedLen(len(streamPath)))
			hex.Encode(streamPathStr, streamPath)
			require.Equal(t, testCase.expectedStreamPath, string(streamPathStr))
			streamName := id.GetStreamName()
			streamNameStr := make([]byte, hex.EncodedLen(len(streamName)))
			hex.Encode(streamNameStr, streamName)
			require.Equal(t, testCase.expectedStreamName, string(streamNameStr))
		})
	}
}

type streamIDMatchesReqConfig struct {
	forwardDestination Destination
	deviceID           uint8
	streamPath         *string
	streamName         *string

	expectedMatch bool
}

func TestStreamIDMatchesReq(t *testing.T) {
	var (
		exampleStreamPath   = testStreamPath
		exampleStreamName   = testStreamName
		differentStreamPath = altStreamPath
		differentStreamName = altStreamName
	)
	key, err := GenerateStreamID(testDestination, testDeviceID, &exampleStreamPath, &exampleStreamName)
	require.Nil(t, err)

	testCases := []streamIDMatchesReqConfig{
		// Exact match
		{testDestination, testDeviceID, &exampleStreamPath, &exampleStreamName, true},

		// Single wildcard
		{DestinationAny, testDeviceID, &exampleStreamPath, &exampleStreamName, true},
		{testDestination, DeviceIDAny, &exampleStreamPath, &exampleStreamName, true},
		{testDestination, testDeviceID, nil, &exampleStreamName, true},
		{testDestination, testDeviceID, &exampleStreamPath, nil, true},

		// // Single different value
		{altDestination, testDeviceID, &exampleStreamPath, &exampleStreamName, false},
		{testDestination, altDeviceID, &exampleStreamPath, &exampleStreamName, false},
		{testDestination, testDeviceID, &differentStreamPath, &exampleStreamName, false},
		{testDestination, testDeviceID, &exampleStreamPath, &differentStreamName, false},
	}

	for idx, testCase := range testCases {
		t.Run(fmt.Sprintf("StreamIDMatchesReq_%d", idx), func(t *testing.T) {
			id, err := GenerateStreamID(testCase.forwardDestination, testCase.deviceID, testCase.streamPath, testCase.streamName)
			require.Nil(t, err)

			isMatch, err := key.Matches(id)
			require.Nil(t, err)
			require.Equal(t, testCase.expectedMatch, isMatch)
		})
	}
}

type formatTestConfig struct {
	formatConfig               *StreamIDFormatConfig
	expectedVersion            uint8
	expectedForwardDestination Destination
	expectedDeviceID           uint8
	expectedStreamPath         string
	expectedStreamName         string
}

func TestFormat(t *testing.T) {
	testCases := []formatTestConfig{
		{&StreamIDFormatConfig{}, uint8(0x3f), Destination(0x7f), uint8(0x7f), "3fffffffffffff", "3fffffffffffff"},

		{&StreamIDFormatConfig{ClearVersion: true}, uint8(0), Destination(0x7f), uint8(0x7f), "3fffffffffffff", "3fffffffffffff"},
		{&StreamIDFormatConfig{ClearForwardingDestination: true}, uint8(0x3f), Destination(0), uint8(0x7f), "3fffffffffffff", "3fffffffffffff"},
		{&StreamIDFormatConfig{ClearDeviceID: true}, uint8(0x3f), Destination(0x7f), uint8(0), "3fffffffffffff", "3fffffffffffff"},
		{&StreamIDFormatConfig{ClearStreamPath: true}, uint8(0x3f), Destination(0x7f), uint8(0x7f), "00000000000000", "3fffffffffffff"},
		{&StreamIDFormatConfig{ClearStreamName: true}, uint8(0x3f), Destination(0x7f), uint8(0x7f), "3fffffffffffff", "00000000000000"},
	}

	for idx, testCase := range testCases {
		t.Run(fmt.Sprintf("Format_%d", idx), func(t *testing.T) {
			sid, err := StreamIDParse("ffffffff-ffff-ffff-ffff-ffffffffffff")
			require.Nil(t, err)
			sid.Format(testCase.formatConfig)

			assert.Equal(t, testCase.expectedVersion, uint8(sid.GetVersion()))
			assert.Equal(t, testCase.expectedForwardDestination, Destination(sid.GetForwardingDestination()))
			assert.Equal(t, testCase.expectedDeviceID, uint8(sid.GetDeviceID()))

			streamPath := sid.GetStreamPath()
			streamPathStr := make([]byte, hex.EncodedLen(len(streamPath)))
			hex.Encode(streamPathStr, streamPath)
			assert.Equal(t, testCase.expectedStreamPath, string(streamPathStr))

			streamName := sid.GetStreamName()
			streamNameStr := make([]byte, hex.EncodedLen(len(streamName)))
			hex.Encode(streamNameStr, streamName)
			assert.Equal(t, testCase.expectedStreamName, string(streamNameStr))
		})
	}
}
