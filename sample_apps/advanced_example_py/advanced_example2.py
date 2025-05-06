# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import json
import os
import sys
import ctypes

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    raise ValueError("JRTC_APP_PATH not set")
sys.path.append(f"{JRTC_APP_PATH}")

import jrtc_app
from jrtc_app import *

generated_data_pb = sys.modules.get('generated_data_pb')
simple_input_pb = sys.modules.get("simple_input_pb")

from generated_data_pb import example_msg_pb
from simple_input_pb import simple_input_pb

##########################################################################
# Define the state variables for the application
class AppStateVars(ctypes.Structure):
    _fields_ = [
        ("app", ctypes.POINTER(JrtcApp)),
        
        # add custom fields below
        ("agg_cnt", ctypes.c_int32),
        ("received_counter", ctypes.c_int)
    ]        

##########################################################################
# Handler callback function (this function gets called by the C library)
def app_handler(timeout: bool, stream_idx: int, data_entry_ptr: ctypes.POINTER(struct_jrtc_router_data_entry), state_ptr: int):

    GENERATOR_PB_OUT_STREAM_IDX = 0
    APP2_OUT_STREAM_IDX = 1
    APP1_IN_STREAM_IDX = 2

    if timeout:
        # Timeout processing (not implemented in this example)
        pass

    else:
        # Dereference the pointer arguments
        state = ctypes.cast(state_ptr, ctypes.POINTER(AppStateVars)).contents        
        data_entry = data_entry_ptr.contents

        output = {
            "dummy": 0,
            "timestamp": 0,
            "stream_idx": stream_idx,
        }
        print("BEFORE")
        a = 10000 * [0]
        for i in range(10000):
            a[i] = 100 * [1]
        print("AFTER")             
        print(f"Testing json.dumps: {json.dumps(output, indent=2)}")        

        if stream_idx == GENERATOR_PB_OUT_STREAM_IDX:

            state.received_counter += 1

            data_ptr = ctypes.cast(
                data_entry.data, ctypes.POINTER(ctypes.c_char)
            )
            raw_data = ctypes.string_at(
                data_ptr, ctypes.sizeof(ctypes.c_char) * 4
            )
            value = int.from_bytes(raw_data, byteorder="little")

            state.agg_cnt += value  # For now, just increment as an example

            print(f"App2: Aggregate counter from codelet is {state.agg_cnt}", flush=True)

            # Get a buffer to write the output data
            counter = simple_input_pb()
            counter.aggregate_counter = state.agg_cnt

            # Send the data to the App2 output channel
            res = jrtc_app_router_channel_send_output(state.app, APP2_OUT_STREAM_IDX, counter.SerializeToString())
            assert res == 0, "Error returned from jrtc_router_channel_send_output"

            # Send the data to the App1 input channel
            aggregate_counter = simple_input_pb()
            aggregate_counter.aggregate_counter = state.agg_cnt

            res = jrtc_app_router_channel_send_input_msg(
                state.app, APP1_IN_STREAM_IDX, aggregate_counter.SerializeToString(), len(aggregate_counter.SerializeToString())
            )
            assert res == 0, "Failure in jrtc_router_channel_send_input_msg"

        else:
            print(f"App1: Got some unexpected message (stream_index={stream_idx})", flush=True)

        if state.received_counter % 5 == 0 and state.received_counter > 0:
            print(f"App2: Aggregate counter so far is {state.agg_cnt}", flush=True)


##########################################################################
# Main function to start the app (converted from jrtc_start_app)
def jrtc_start_app(capsule):

    streams = [
        # GENERATOR_PB_OUT_STREAM_IDX = 0
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY, 
                JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
                b"AdvancedExample2://jbpf_agent/data_generator_pb_codeletset/codelet", 
                b"ringbuf"),
            True,   # is_rx
            None    # No AppChannelCfg 
        ),
        # APP2_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE, 
                0, 
                b"AdvancedExample2://intraapp", 
                b"buffer"),
            False,  # is_rx
            None    # No AppChannelCfg 
        ),
        # APP1_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE, 
                0, 
                b"AdvancedExample1://intraapp", 
                b"buffer_input"),
            False,  # is_rx
            None    # No AppChannelCfg 
        )
    ]

    app_cfg = JrtcAppCfg_t(
        b"AdvancedExample2",                   # context
        100,                                  # q_size
        len(streams),                         # num_streams
        (JrtcStreamCfg_t * len(streams))(*streams), # streams
        10.0,                                 # initialization_timeout_secs
        0.1,                                  # sleep_timeout_secs
        1.0                                   # inactivity_timeout_secs
    )

    # Initialize the app
    state = AppStateVars(agg_cnt=0, received_counter=0)
    state.app = jrtc_app_create(capsule, app_cfg, app_handler, state)

    # run the app - This is blocking until the app exists
    jrtc_app_run(state.app)

    # clean up app resources
    jrtc_app_destroy(state.app)
