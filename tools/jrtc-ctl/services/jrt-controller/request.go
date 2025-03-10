// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jrtc

import (
	"fmt"
	"os"
	"time"

	openapi_types "github.com/oapi-codegen/runtime/types"
)

// NewJrtcAppLoadRequest creates a new JrtcAppLoadRequest
func NewJrtcAppLoadRequest(
	sharedLibraryPath, appName string,
	ioqSize int32,
	deadline, period, runtime time.Duration,
	appType string,
	appParams *map[string]interface{},
) (*JrtcAppLoadRequest, error) {
	fi, err := os.Stat(sharedLibraryPath)
	if err != nil {
		return nil, err
	} else if fi.IsDir() {
		return nil, fmt.Errorf(`expected "%s" to be a file, got directory`, sharedLibraryPath)
	}
	sharedObject, err := os.ReadFile(sharedLibraryPath)
	if err != nil {
		return nil, err
	}

	var f openapi_types.File
	f.InitFromBytes(sharedObject, sharedLibraryPath)

	return &JrtcAppLoadRequest{
		App:        f,
		AppName:    appName,
		DeadlineUs: int32(deadline.Microseconds()),
		IoqSize:    ioqSize,
		PeriodUs:   int32(period.Microseconds()),
		RuntimeUs:  int32(runtime.Microseconds()),
		AppPath:    &sharedLibraryPath,
		AppType:    &appType,
		AppParams:  appParams,
	}, nil
}

// NewJrtcAppLoadRequestFromBytes creates a new JrtcAppLoadRequest using byte array
func NewJrtcAppLoadRequestFromBytes(
	sharedLibraryCode []byte, sharedLibraryPath, appName string,
	ioqSize int32,
	deadline, period, runtime time.Duration,
	appType string,
	appParams *map[string]interface{},
) (*JrtcAppLoadRequest, error) {
	var f openapi_types.File
	f.InitFromBytes(sharedLibraryCode, sharedLibraryPath)

	return &JrtcAppLoadRequest{
		App:        f,
		AppName:    appName,
		DeadlineUs: int32(deadline.Microseconds()),
		IoqSize:    ioqSize,
		PeriodUs:   int32(period.Microseconds()),
		RuntimeUs:  int32(runtime.Microseconds()),
		AppPath:    &sharedLibraryPath,
		AppType:    &appType,
		AppParams:  appParams,
	}, nil
}
