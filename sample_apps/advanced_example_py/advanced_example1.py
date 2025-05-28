# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.
import os
import sys
import ctypes
from dataclasses import dataclass

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    raise ValueError("JRTC_APP_PATH not set")
sys.path.append(f"{JRTC_APP_PATH}")

from jrtc_app import *

generated_data = sys.modules.get("generated_data")
simple_input_pb = sys.modules.get("simple_input_pb")

from generated_data import example_msg
from simple_input_pb import simple_input_pb


##########################################################################
# Define the state variables for the application
@dataclass
class AppStateVars:
    app: JrtcApp
    agg_cnt: int


##########################################################################
def app_handler(
    timeout: bool,
    stream_idx: int,
    data_entry: struct_jrtc_router_data_entry,
    state: AppStateVars,
):
    GENERATOR_OUT_STREAM_IDX = 0
    APP2_OUT_STREAM_IDX = 1
    SIMPLE_INPUT_CODELET_IN_STREAM_IDX = 2
    APP1_IN_STREAM_IDX = 3

    if timeout:
        # Timeout processing (not implemented in this example)
        return

    if stream_idx == GENERATOR_OUT_STREAM_IDX:
        # Extract data from the received entry
        data = ctypes.cast(data_entry.data, ctypes.POINTER(example_msg)).contents
        state.agg_cnt += data.cnt
        print(f"App1: Aggregate counter from codelet is {state.agg_cnt}", flush=True)

        data_to_send = bytes(state.agg_cnt)
        data_len = ctypes.sizeof(simple_input_pb)

        # Send the aggregate counter back to the input codelet
        res = jrtc_app_router_channel_send_input_msg(
            state.app, SIMPLE_INPUT_CODELET_IN_STREAM_IDX, data_to_send, data_len
        )
        assert res == 0, "Failure in jrtc_router_channel_send_input_msg"
        print(
            f"App1: Sent aggregate counter {state.agg_cnt} to input codelet",
            flush=True,
        )

    elif stream_idx == APP2_OUT_STREAM_IDX:
        # Data received from App2 output channel
        appdata = ctypes.cast(data_entry.data, ctypes.POINTER(simple_input_pb)).contents
        print(
            f"App1: Received aggregate counter {appdata.aggregate_counter} from output channel of App2",
            flush=True,
        )

    elif stream_idx == APP1_IN_STREAM_IDX:
        # Data received from App1 input channel (by App2)
        appdata = ctypes.cast(data_entry.data, ctypes.POINTER(simple_input_pb)).contents
        print(
            f"App1: Received aggregate counter {appdata.aggregate_counter} from input channel of App1",
            flush=True,
        )

    else:
        print(
            f"App1: Got some unexpected message (stream_index={stream_idx})",
            flush=True,
        )
        assert False


##########################################################################
# Main function to start the app (converted from jrtc_start_app)
def jrtc_start_app(capsule):
    streams = [
        # GENERATOR_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY,
                JRTC_ROUTER_REQ_DEVICE_ID_ANY,
                b"AdvancedExample1://jbpf_agent/data_generator_codeletset/codelet",
                b"ringbuf",
            ),
            True,  # is_rx
            None,  # No AppChannelCfg
        ),
        # APP2_OUT_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_ANY, 0, b"AdvancedExample2://intraapp", b"buffer"
            ),
            True,  # is_rx
            None,  # No AppChannelCfg
        ),
        # SIMPLE_INPUT_CODELET_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE,
                1,
                b"AdvancedExample1://jbpf_agent/simple_input_pb_codeletset/codelet",
                b"input_map",
            ),
            False,  # is_rx
            None,  # No AppChannelCfg
        ),
        # APP1_IN_STREAM_IDX
        JrtcStreamCfg_t(
            JrtcStreamIdCfg_t(
                JRTC_ROUTER_REQ_DEST_NONE,
                0,
                b"AdvancedExample1://intraapp",
                b"buffer_input",
            ),
            True,  # is_rx
            ctypes.POINTER(JrtcAppChannelCfg_t)(
                JrtcAppChannelCfg_t(False, 100, ctypes.sizeof(simple_input_pb))
            ),
        ),
    ]

    app_cfg = JrtcAppCfg_t(
        b"AdvancedExample1",  # context
        100,  # q_size
        len(streams),  # num_streams
        (JrtcStreamCfg_t * len(streams))(*streams),  # streams
        10.0,  # initialization_timeout_secs
        0.1,  # sleep_timeout_secs
        1.0,  # inactivity_timeout_secs
    )

    # Initialize the app
    state = AppStateVars(agg_cnt=0, app=None)
    state.app = jrtc_app_create(capsule, app_cfg, app_handler, state)

    # run the app - This is blocking until the app exits
    jrtc_app_run(state.app)

    # clean up app resources
    jrtc_app_destroy(state.app)
