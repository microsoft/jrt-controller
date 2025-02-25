// Copyright (c) Microsoft Corporation. All rights reserved.

package jrtcbindings

// #include <stdlib.h>
// #include "jrtc_router_stream_id.h"
import "C"

const (
	// DeviceIDAny is the value representing any device ID
	DeviceIDAny uint8 = C.JRTC_ROUTER_DEVICE_ID_ANY
)
