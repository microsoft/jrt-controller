# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import time
import os
import sys
import ctypes
from dataclasses import dataclass

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    raise ValueError("JRTC_APP_PATH not set")
sys.path.append(f"{JRTC_APP_PATH}")

from jrtc_wrapper_utils import get_ctx_from_capsule
import jrtc_app
from jrtc_app import *

generated_data = sys.modules.get("generated_data")
simple_input = sys.modules.get("simple_input")

from simple_input import simple_input
from jrtc_bindings import (
    struct_jrtc_router_data_entry,
)


##########################################################################
# Define the state variables for the application
@dataclass
class AppStateVars:
    app: JrtcApp
    agg_cnt: int
    received_counter: int


##########################################################################
def app_handler(
    timeout: bool,
    stream_idx: int,
    data_entry: struct_jrtc_router_data_entry,
    state: AppStateVars,
):
    GENERATOR_OUT_STREAM_IDX = 0
    SIMPLE_INPUT_IN_STREAM_IDX = 1

    if timeout:
        # Timeout processing (not implemented in this example)
        return

    if stream_idx == GENERATOR_OUT_STREAM_IDX:
        state.received_counter += 1

        data_ptr = ctypes.cast(data_entry.data, ctypes.POINTER(ctypes.c_char))
        raw_data = ctypes.string_at(data_ptr, ctypes.sizeof(ctypes.c_char) * 4)
        value = int.from_bytes(raw_data, byteorder="little")

        state.agg_cnt += value  # For now, just increment as an example

    if state.received_counter % 5 == 0 and state.received_counter > 0:
        aggregate_counter = simple_input()
        aggregate_counter.aggregate_counter = state.agg_cnt
        data_to_send = bytes(aggregate_counter)

        # send the data
        res = jrtc_app_router_channel_send_input_msg(
            state.app, SIMPLE_INPUT_IN_STREAM_IDX, data_to_send, len(data_to_send)
        )
        assert res == 0, "Failed to send aggregate counter to input map"

        print(
            f"FirstExample: Aggregate counter so far is: {state.agg_cnt}",
            flush=True,
        )


##########################################################################
# Main function to start the app (converted from jrtc_start_app)
def jrtc_start_app(capsule):
    print("Starting FirstExample app...", flush=True)
    env_ctx = get_ctx_from_capsule(capsule)
    if not env_ctx:
        raise ValueError("Failed to retrieve JrtcAppEnv from capsule")

    ## Extract necessary fields from env_ctx and make assertions
    app_name = env_ctx.app_name.decode("utf-8")
    io_queue_size = env_ctx.io_queue_size
    app_path = env_ctx.app_path.decode("utf-8")
    app_modules = env_ctx.app_modules
    app_params = env_ctx.params
    device_mapping = env_ctx.device_mapping

    ## assertion
    assert app_name == "app1", f"Unexpected app name: {app_name}"
    assert io_queue_size == 1000, f"Unexpected IO queue size: {io_queue_size}"
    assert app_path.split("/")[-1] == "libjrtc_pythonapp_loader.so", f"Unexpected app path: {app_path}"
    assert app_modules[0].decode("utf-8").split('/')[-1] == "generated_data.py", f"Unexpected first module: {app_modules[0]}"
    assert app_modules[1].decode("utf-8").split('/')[-1] == "simple_input.py", f"Unexpected second module: {app_modules[1]}"
    assert app_params[0].key.decode("utf-8") == "python", f"Unexpected app parameter key: {app_params[0].key}"
    assert app_params[0].value.decode("utf-8").split('/')[-1] == "first_example.py", f"Unexpected app parameter value: {app_params[0].value}"
    assert device_mapping[0].key.decode("utf-8") == "1", f"Unexpected device mapping key: {device_mapping[0].key}"
    assert device_mapping[0].value.decode("utf-8") == "127.0.0.1:8080", f"Unexpected device mapping value: {device_mapping[0].value}"
    
    streams = [
        # GENERATOR_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY,
                JRTC_ROUTER_REQ_DEVICE_ID_ANY,
                b"FirstExample://jbpf_agent/data_generator_codeletset/codelet",
                b"ringbuf",
            ),
            True,  # is_rx
            None,  # No AppChannelCfg
        ),
        # SIMPLE_INPUT_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE,
                1,
                b"FirstExample://jbpf_agent/simple_input_codeletset/codelet",
                b"input_map",
            ),
            False,  # is_rx
            None,  # No AppChannelCfg
        ),
    ]

    app_cfg = JrtcAppCfg_t(
        b"FirstExample",  # context
        100,  # q_size
        len(streams),  # num_streams
        (JrtcStreamCfg_t * len(streams))(*streams),  # streams
        10.0,  # initialization_timeout_secs
        0.1,  # sleep_timeout_secs
        1.0,  # inactivity_timeout_secs
    )

    # Initialize the app
    state = AppStateVars(agg_cnt=0, received_counter=0, app=None)
    state.app = jrtc_app_create(capsule, app_cfg, app_handler, state, log_level="INFO")

    # run the app - This is blocking until the app exists
    jrtc_app_run(state.app)

    # clean up app resources
    jrtc_app_destroy(state.app)


    ## simulate the app exit taking too much time
    print("Simulating app exit delay...", flush=True)
    time.sleep(10)
    print("FirstExample app has exited.", flush=True)
