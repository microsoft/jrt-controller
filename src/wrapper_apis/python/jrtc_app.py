# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.


import time
import os
import sys
import ctypes
from ctypes import c_int, c_float, c_char_p, c_void_p, c_bool
from dataclasses import dataclass
from typing import Any

# Retrieve the JRTC application path from environment variables
JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")
from jrtc_bindings import *

# Import necessary JRTC modules
from jrtc_router_stream_id import (
    JrtcRouterStreamId,
    jrtc_router_stream_id_matches_req
)
from jrtc_wrapper_utils import JrtcAppEnv, get_ctx_from_capsule, get_data_entry_array_ptr
from jrtc_router_lib import (
    jrtc_router_channel_register_stream_id_req,
    jrtc_router_channel_deregister_stream_id_req,
    jrtc_router_receive,
    jrtc_router_channel_send_input_msg,    
    jrtc_router_channel_send_output_msg,
    jrtc_router_channel_create,    
    jrtc_router_channel_destroy,    
    jrtc_router_channel_release_buf,
    jrtc_router_input_channel_exists,
    JRTC_ROUTER_REQ_DEST_ANY,
    JRTC_ROUTER_REQ_DEVICE_ID_ANY,
    JRTC_ROUTER_REQ_DEST_NONE,
    JRTC_ROUTER_REQ_STREAM_PATH_ANY,
    JRTC_ROUTER_REQ_STREAM_NAME_ANY
)



# Load the shared C library
lib = ctypes.CDLL(f'{JRTC_APP_PATH}/libjrtc_app.so')

# Define C structures (using ctypes)

class JrtcApp(ctypes.Structure):
    pass

class JrtcStreamIdCfg_t(ctypes.Structure):
    _fields_ = [
        ("destination", c_int),
        ("device_id", c_int),
        ("stream_source", c_char_p),
        ("io_map", c_char_p)
    ]

class JrtcAppChannelCfg_t(ctypes.Structure):
    _fields_ = [
        ("is_output", c_bool),
        ("num_elems", c_int),
        ("elem_size", c_int)
    ]

class JrtcStreamCfg_t(ctypes.Structure):
    _fields_ = [
        ("sid", JrtcStreamIdCfg_t),
        ("is_rx", c_bool),
        ("appChannel", ctypes.POINTER(JrtcAppChannelCfg_t))
    ]

class JrtcAppCfg_t(ctypes.Structure):
    _fields_ = [
        ("context", c_char_p),
        ("q_size", c_int),
        ("num_streams", c_int),
        ("streams", ctypes.POINTER(JrtcStreamCfg_t)),
        ("sleep_timeout_secs", c_float),
        ("inactivity_timeout_secs", c_float)
    ]

# Callback type definition for the handler function

JrtcAppHandler = ctypes.CFUNCTYPE(None, c_bool, c_int, ctypes.POINTER(struct_jrtc_router_data_entry), c_void_p)

# Function prototypes

lib.jrtc_app_create.argtypes = [ctypes.POINTER(JrtcAppEnv), ctypes.POINTER(JrtcAppCfg_t), JrtcAppHandler, c_void_p]
lib.jrtc_app_create.restype = ctypes.POINTER(JrtcApp) 

#lib.jrtc_app_destroy.argtypes = [c_void_p]
lib.jrtc_app_destroy.argtypes = [ctypes.POINTER(JrtcApp)]
lib.jrtc_app_destroy.restype = None

lib.jrtc_app_run.argtypes = [ctypes.POINTER(JrtcApp)]
lib.jrtc_app_run.restype = None

lib.jrtc_app_router_channel_send_input_msg.argtypes = [ctypes.POINTER(JrtcApp), c_int, c_void_p, c_int]
lib.jrtc_app_router_channel_send_input_msg.restype = c_int

lib.jrtc_app_router_channel_send_output_msg.argtypes = [ctypes.POINTER(JrtcApp), c_int, c_void_p, c_int]
lib.jrtc_app_router_channel_send_output_msg.restype = c_int



#######################################################################
# Python wrapper to call jrtc_app_create C function
def jrtc_app_create(capsule, app_cfg: JrtcAppCfg_t, app_handler: JrtcAppHandler, app_state: Any) -> None :

    env_ctx = get_ctx_from_capsule(capsule)
    app_handler_c =  JrtcAppHandler(app_handler)

    print("app_handler_c = ", app_handler_c)
    app_handler_raw = ctypes.cast(app_handler_c, ctypes.c_void_p)

    return lib.jrtc_app_create(
        ctypes.byref(env_ctx), 
        ctypes.byref(app_cfg), 
        app_handler_c,
        ctypes.pointer(app_state))

    
#######################################################################
# Python wrapper to call jrtc_app_run C function
def jrtc_app_run(app: ctypes.POINTER(JrtcApp)) -> None:
    lib.jrtc_app_run(app)


#######################################################################
# Python wrapper to call jrtc_app_destroy 
def jrtc_app_destroy(app: ctypes.POINTER(JrtcApp)) -> None:
    lib.jrtc_app_destroy(app)


#######################################################################
# Python wrapper to call jrtc_router_channel_send_input_msg 
def jrtc_app_router_channel_send_input_msg(app: ctypes.POINTER(JrtcApp), stream_idx: int, data: bytes, data_len: int) -> int:
    return lib.jrtc_app_router_channel_send_input_msg(app, stream_idx, data, data_len)


#######################################################################
# Python wrapper to call jrtc_router_channel_send_input_msg 
def jrtc_app_router_channel_send_output_msg(app: ctypes.POINTER(JrtcApp), stream_idx: int, data: bytes, data_len: int) -> int:
    return lib.jrtc_app_router_channel_send_output_msg(app, stream_idx, data, data_len)





########################################################
# Lists of items to expose when this module is importd
########################################################
__all__ = [ 
    'JRTC_ROUTER_REQ_DEST_ANY',
    'JRTC_ROUTER_REQ_DEVICE_ID_ANY',
    'JRTC_ROUTER_REQ_DEST_NONE',
    'JRTC_ROUTER_REQ_STREAM_PATH_ANY',
    'JRTC_ROUTER_REQ_STREAM_NAME_ANY',
    'struct_jrtc_router_data_entry',

    'JrtcStreamIdCfg_t',
    'JrtcAppChannelCfg_t',
    'JrtcStreamCfg_t',
    'JrtcAppCfg_t',
    
    'JrtcApp',
    'jrtc_app_create',
    'jrtc_app_run',
    'jrtc_app_destroy',
    'jrtc_app_router_channel_send_input_msg',
    'jrtc_app_router_channel_send_output_msg',

    ]
