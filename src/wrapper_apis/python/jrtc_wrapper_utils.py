# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import ctypes
import os
import sys


JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")
import jrtc_bindings


def register_dll(path):
    try:
        module = ctypes.CDLL(path)
        print(f"Loaded {path} successfuly.")
        return module
    except OSError as e:
        raise ImportError(f"Failed to load {path} {e}")
        return None

class KeyValuePair(ctypes.Structure):
    _fields_ = [
        ("key", ctypes.c_char_p),
        ("value", ctypes.c_char_p),
    ]

MAX_APP_PARAMS = 255
MAX_DEVICE_MAPPING = 255
MAX_APP_MODULES = 255

class JrtcAppEnv(ctypes.Structure):
    _fields_ = [
        ("app_name", ctypes.c_char_p),  # Define as c_char_p
        ("app_tid", ctypes.c_ulong),
        ("app_id", ctypes.c_ulong),
        ("app_handle", ctypes.POINTER(None)),
        ("dapp_ctx", jrtc_bindings.dapp_router_ctx_t),
        ("io_queue_size", ctypes.c_uint),
        ("app_exit", ctypes.c_int),
        ("sched_config", jrtc_bindings.struct_jrtc_sched_config),
        ("app_path", ctypes.c_char_p),
        ("params", KeyValuePair * MAX_APP_PARAMS),
        ("device_mapping", KeyValuePair * MAX_DEVICE_MAPPING),
        ("app_modules", ctypes.c_char_p * MAX_APP_MODULES),
        ("shared_python_state", ctypes.c_void_p),
    ]

def get_ctx_from_capsule(capsule):
    ctypes.pythonapi.PyCapsule_GetPointer.restype = ctypes.c_void_p
    ctypes.pythonapi.PyCapsule_GetPointer.argtypes = [ctypes.py_object, ctypes.c_char_p]

    try:
        env_ctx_ptr = ctypes.pythonapi.PyCapsule_GetPointer(capsule, b"void*")
    except Exception as e:
        raise ValueError(f"Failed to retrieve pointer: {e}")

    if not env_ctx_ptr:
        raise ValueError("Failed to retrieve pointer from PyCapsule.")

    # Cast the pointer to the JrtcAppEnv struct
    env_ctx = ctypes.cast(env_ctx_ptr, ctypes.POINTER(JrtcAppEnv)).contents    
    return env_ctx

def get_data_entry_array_ptr(num_of_entries):
    JrtcRouterDataEntryArray = (
        jrtc_bindings.struct_jrtc_router_data_entry * num_of_entries
    )
    data_entries_array = JrtcRouterDataEntryArray()
    data_entries_array_ptr = ctypes.cast(
        data_entries_array,
        ctypes.POINTER(jrtc_bindings.struct_jrtc_router_data_entry),
    )
    return data_entries_array_ptr
