# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import os
import ctypes
import sys

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")

from jrtc_wrapper_utils import register_dll
from jrtc_router_stream_id import JrtcRouterStreamId
import jrtc_bindings

jrtc_router_lib_path = os.path.join(JRTC_APP_PATH, "liblibjrtc_router.so")
jrtc_router_lib = register_dll(jrtc_router_lib_path)

JRTC_ROUTER_REQ_DEST_ANY = 0x7F
JRTC_ROUTER_REQ_DEVICE_ID_ANY = 0x7F
JRTC_ROUTER_REQ_DEST_NONE = 0x1
JRTC_ROUTER_REQ_STREAM_PATH_ANY = None
JRTC_ROUTER_REQ_STREAM_NAME_ANY = None
JRTC_ROUTER_CONTROLLER_DEVICE_ID = 0x0

def jrtc_router_receive(app_ctx, data_entries_array_ptr, num_entries):
    jrtc_router_lib.jrtc_router_receive.argtypes = [
        ctypes.POINTER(jrtc_bindings.struct_dapp_router_ctx),  # app_ctx
        ctypes.POINTER(jrtc_bindings.struct_jrtc_router_data_entry),  # data_entries
        ctypes.c_size_t,  # num_entries
    ]
    jrtc_router_lib.jrtc_router_receive.restype = ctypes.c_int

    res = jrtc_router_lib.jrtc_router_receive(
        app_ctx, data_entries_array_ptr, num_entries
    )
    return res

def jrtc_router_channel_register_stream_id_req(dapp_ctx, stream_id):
    jrtc_router_lib.jrtc_router_channel_register_stream_id_req.argtypes = [
        jrtc_bindings.dapp_router_ctx_t,
        jrtc_bindings.struct_jrtc_router_stream_id,
    ]
    jrtc_router_lib.jrtc_router_channel_register_stream_id_req.restype = (
        ctypes.c_int
    )
    return jrtc_router_lib.jrtc_router_channel_register_stream_id_req(
        dapp_ctx, stream_id
    )

def jrtc_router_channel_deregister_stream_id_req(dapp_ctx, stream_id):
    jrtc_router_lib.jrtc_router_channel_deregister_stream_id_req.argtypes = [
        jrtc_bindings.dapp_router_ctx_t,
        jrtc_bindings.struct_jrtc_router_stream_id,
    ]
    jrtc_router_lib.jrtc_router_channel_deregister_stream_id_req.restype = None
    jrtc_router_lib.jrtc_router_channel_deregister_stream_id_req(
        dapp_ctx, stream_id
    )

def jrtc_router_channel_send_input_msg(stream_id, data, data_len):
    jrtc_router_lib.jrtc_router_channel_send_input_msg.argtypes = [
        jrtc_bindings.struct_jrtc_router_stream_id,  # stream_id
        ctypes.c_void_p,  # data
        ctypes.c_size_t,  # data_len
    ]
    jrtc_router_lib.jrtc_router_channel_send_input_msg.restype = ctypes.c_int
    return jrtc_router_lib.jrtc_router_channel_send_input_msg(stream_id, data, data_len)

def jrtc_router_channel_send_output_msg(chan_ctx, data, data_len):
    jrtc_router_lib.jrtc_router_channel_send_output_msg.argtypes = [
        jrtc_bindings.dapp_channel_ctx_t,  # chan_ctx
        ctypes.c_void_p,  # data
        ctypes.c_size_t,  # data_len
    ]
    jrtc_router_lib.jrtc_router_channel_send_output_msg.restype = ctypes.c_int
    return jrtc_router_lib.jrtc_router_channel_send_output_msg(chan_ctx, data, data_len)

def jrtc_router_channel_release_buf(ptr):
    jrtc_router_lib.jrtc_router_channel_release_buf.argtypes = [
        ctypes.c_void_p,  # ptr
    ]
    jrtc_router_lib.jrtc_router_channel_release_buf.restype = None
    return jrtc_router_lib.jrtc_router_channel_release_buf(ptr)

def jrtc_router_channel_create(dapp_ctx, is_output, num_elems, elem_size, stream_id, descriptor, descriptor_size):
    jrtc_router_lib.jrtc_router_channel_create.argtypes = [
        jrtc_bindings.dapp_router_ctx_t,   # dapp_ctx
        ctypes.c_bool, # is_output
        ctypes.c_int, # num_elems
        ctypes.c_int, # elem_size
        JrtcRouterStreamId,  # stream_id
        ctypes.c_void_p, # descriptor
        ctypes.c_size_t # descriptor_size
    ]
    jrtc_router_lib.jrtc_router_channel_create.restype = jrtc_bindings.dapp_channel_ctx_t
    return jrtc_router_lib.jrtc_router_channel_create(dapp_ctx, is_output, num_elems, elem_size, stream_id, descriptor, descriptor_size)

def jrtc_router_channel_destroy(chan_ctx):
    jrtc_router_lib.jrtc_router_channel_destroy.argtypes = [
        jrtc_bindings.dapp_channel_ctx_t   # chan_ctx
    ]
    jrtc_router_lib.jrtc_router_channel_destroy.restype = None
    jrtc_router_lib.jrtc_router_channel_destroy(chan_ctx)
    
def jrtc_router_input_channel_exists(stream_id):
    jrtc_router_lib.jrtc_router_input_channel_exists.argtypes = [
        jrtc_bindings.struct_jrtc_router_stream_id,  # stream_id
    ]
    jrtc_router_lib.jrtc_router_input_channel_exists.restype = ctypes.c_int
    return jrtc_router_lib.jrtc_router_input_channel_exists(stream_id)
