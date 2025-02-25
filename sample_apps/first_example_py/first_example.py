# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import time
import os
import sys
import ctypes
from dataclasses import dataclass

JRTC_PYTHON_INC = os.environ.get("JRTC_PYTHON_INC")
if JRTC_PYTHON_INC is None:
    raise ValueError("JRTC_PYTHON_INC not set")
sys.path.append(f"{JRTC_PYTHON_INC}")

import jrtc_app
from jrtc_app import *

JRTC_PATH = f'{os.environ.get("JRTC_PATH")}'
if JRTC_PATH is None:
    raise ValueError("JRTC_PATH not set")

sys.path.append(f"{JRTC_PATH}/sample_apps/first_example_py/jbpf_codelets/data_generator/")
sys.path.append(f"{JRTC_PATH}/sample_apps/first_example_py/jbpf_codelets/simple_input/")
from generated_data import example_msg
from simple_input import simple_input


########################################################
# state variables
########################################################
@dataclass
class AppStateVars:
    wrapper: JrtcPythonApp
    agg_cnt: int
    received_counter: int


# ######################################################
# # Main function called from JRTC core
def jrtc_start_app(capsule):

    jrtc_device_id = 1

    GENERATOR_OUT_STREAM_IDX = 0
    SIMPLE_INPUT_IN_STREAM_IDX = 1

    ########################################################
    # configuration
    ########################################################
    app_cfg = AppCfg(
        context="FirstExample",
        q_size=100, 
        streams=[
            StreamCfg(
                sid=StreamIdCfg(
                    destination=JRTC_ROUTER_REQ_DEST_ANY, 
                    device_id=JRTC_ROUTER_REQ_DEVICE_ID_ANY, 
                    stream_source="FirstExample://jbpf_agent/data_generator_codeletset/codelet1", 
                    io_map="ringbuf"),
                is_rx=True
            ),
            StreamCfg(
                sid=StreamIdCfg(
                    destination=JRTC_ROUTER_REQ_DEST_NONE, 
                    device_id=jrtc_device_id, 
                    stream_source="FirstExample://jbpf_agent/unique_id_for_codelet_simple_input/codelet1", 
                    io_map="input_map"),
                is_rx=False
            )
        ], 
        sleep_timeout_secs=0.1,
        inactivity_timeout_secs=1.0
    )

    ########################################################
    # message handler
    ########################################################
    def app_handler(timeout: bool, stream_idx: int, data_entry: struct_jrtc_router_data_entry, state: AppStateVars) -> None:

        if timeout:

            ## timeout processing

            pass

        elif stream_idx == GENERATOR_OUT_STREAM_IDX:

            state.received_counter += 1

            data_ptr = ctypes.cast(
                data_entry.data, ctypes.POINTER(ctypes.c_char)
            )
            raw_data = ctypes.string_at(
                data_ptr, ctypes.sizeof(ctypes.c_char) * 4
            )
            value = int.from_bytes(raw_data, byteorder="little")
            state.agg_cnt += value
            
        if state.received_counter % 5 == 0 and state.received_counter > 0:
            print(f"App 1: Aggregate counter so far is: {state.agg_cnt}")

            aggregate_counter = simple_input()
            aggregate_counter.aggregate_counter = state.agg_cnt
            data_to_send = bytes(aggregate_counter)

            # get the inout stream
            simple_input_stream = jrtc_app_get_stream(state.wrapper, SIMPLE_INPUT_IN_STREAM_IDX)
            assert (simple_input_stream is not None), "Failed to get SIMPLE_INPUT_IN_STREAM_IDX stream"

            # send the data
            res = jrtc_router_channel_send_input_msg(simple_input_stream, data_to_send, len(data_to_send))
            assert res == 0, "Failed to send aggregate counter to input map"

        else:
            pass

    ########################################################
    # App execution handler
    ########################################################
    state = AppStateVars(wrapper=None, agg_cnt=0, received_counter=0)
    state.wrapper = JrtcPythonApp(capsule, app_cfg, app_handler, state)
    state.wrapper.run()