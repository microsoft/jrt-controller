# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import os
import sys
import ctypes

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    raise ValueError("JRTC_APP_PATH not set")
sys.path.append(f"{JRTC_APP_PATH}")

import jrtc_app
from jrtc_app import *

generated_data = sys.modules.get('generated_data')
simple_input = sys.modules.get('simple_input')
from generated_data import example_msg
from simple_input import simple_input

##########################################################################
# Define the state variables for the application
class AppStateVars(ctypes.Structure):
    _fields_ = [
        ("app", ctypes.py_object),
        
        # add custom fields below
        ("agg_cnt", ctypes.c_int),
        ("received_counter", ctypes.c_int)
    ]        


##########################################################################
# Handler callback function (this function gets called by the C library)
def app_handler(timeout: bool, stream_idx: int, data_entry_ptr: ctypes.POINTER(struct_jrtc_router_data_entry), state_ptr: int):

    GENERATOR_OUT_STREAM_IDX = 0
    SIMPLE_INPUT_IN_STREAM_IDX = 1

    if timeout:
        # Timeout processing (not implemented in this example)
        pass

    else:

        # Dereference the pointer arguments
        state = ctypes.cast(state_ptr, ctypes.POINTER(AppStateVars)).contents        
        data_entry = data_entry_ptr.contents

        if stream_idx == GENERATOR_OUT_STREAM_IDX:

            state.received_counter += 1

            data_ptr = ctypes.cast(
                data_entry.data, ctypes.POINTER(ctypes.c_char)
            )
            raw_data = ctypes.string_at(
                data_ptr, ctypes.sizeof(ctypes.c_char) * 4
            )
            value = int.from_bytes(raw_data, byteorder="little")

            state.agg_cnt += value  # For now, just increment as an example

        if state.received_counter % 5 == 0 and state.received_counter > 0:

            aggregate_counter = simple_input()
            aggregate_counter.aggregate_counter = state.agg_cnt
            data_to_send = bytes(aggregate_counter)

            # send the data
            res = jrtc_app_router_channel_send_input_msg(state.app, SIMPLE_INPUT_IN_STREAM_IDX, data_to_send, len(data_to_send))
            assert res == 0, "Failed to send aggregate counter to input map"

            print(f"FirstExample: Aggregate counter so far is: {state.agg_cnt}", flush=True)


##########################################################################
# Main function to start the app (converted from jrtc_start_app)
def jrtc_start_app(capsule):

    streams = [
        # GENERATOR_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY, 
                JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
                b"FirstExample://jbpf_agent/data_generator_codeletset/codelet", 
                b"ringbuf"),
            True,   # is_rx
            None    # No AppChannelCfg 
        ),
        # SIMPLE_INPUT_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE, 
                1, 
                b"FirstExample://jbpf_agent/simple_input_codeletset/codelet", 
                b"input_map"),
            False,  # is_rx
            None    # No AppChannelCfg 
        )
    ]

    app_cfg = JrtcAppCfg_t(
        b"FirstExample",                               # context
        100,                                           # q_size
        len(streams),                                  # num_streams
        (JrtcStreamCfg_t * len(streams))(*streams),    # streams
        10.0,                                          # initialization_timeout_secs
        0.1,                                           # sleep_timeout_secs
        1.0                                            # inactivity_timeout_secs
    )

    # Initialize the app
    state = AppStateVars(agg_cnt=0, received_counter=0)
    state.app = jrtc_app_create(capsule, app_cfg, app_handler, state)

    # run the app - This is blocking until the app exists
    jrtc_app_run(state.app)

    # clean up app resources
    jrtc_app_destroy(state.app)

