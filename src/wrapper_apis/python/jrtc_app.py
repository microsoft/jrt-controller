# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import time
import os
import sys
import ctypes
from ctypes import (
    c_int, c_float, c_char_p, c_void_p, c_bool,
    POINTER, Structure, c_uint16, c_uint64
)
from dataclasses import dataclass
from typing import Any, Optional

# Retrieve the JRTC application path from environment variables
JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")

from jrtc_bindings import *
from jrtc_router_stream_id import JrtcRouterStreamId, jrtc_router_stream_id_matches_req
from jrtc_wrapper_utils import (
    JrtcAppEnv,
    get_ctx_from_capsule,
    get_data_entry_array_ptr,
)
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
    JRTC_ROUTER_REQ_STREAM_NAME_ANY,
)

# Structs
class JrtcStreamIdCfg_t(Structure):
    _fields_ = [
        ("destination", c_int),
        ("device_id", c_int),
        ("stream_source", c_char_p),
        ("io_map", c_char_p),
    ]

class JrtcAppChannelCfg_t(Structure):
    _fields_ = [("is_output", c_bool), ("num_elems", c_int), ("elem_size", c_int)]

class JrtcStreamCfg_t(Structure):
    _fields_ = [
        ("sid", JrtcStreamIdCfg_t),
        ("is_rx", c_bool),
        ("appChannel", POINTER(JrtcAppChannelCfg_t)),
    ]

class JrtcAppCfg_t(Structure):
    _fields_ = [
        ("context", c_char_p),
        ("q_size", c_int),
        ("num_streams", c_int),
        ("streams", POINTER(JrtcStreamCfg_t)),
        ("initialization_timeout_secs", c_float),
        ("sleep_timeout_secs", c_float),
        ("inactivity_timeout_secs", c_float),
    ]

# Opaque types
class ChannelCtx(ctypes.Structure): pass
class RouterDataEntry(ctypes.Structure): pass

# Callback type definition
JrtcAppHandler = ctypes.CFUNCTYPE(
    None, c_bool, c_int, POINTER(struct_jrtc_router_data_entry), c_void_p
)

class StreamItem(ctypes.Structure):
    sid: Optional[JrtcRouterStreamId] = None
    registered: bool = False
    chan_ctx: Optional[ChannelCtx] = None

class AppStateVars(ctypes.Structure):
    pass

class JrtcAppCStruct(Structure):
    _fields_ = [
        ("env_ctx", JrtcAppEnv),
        ("app_cfg", JrtcAppCfg_t),
        ("app_handler", JrtcAppHandler),
        ("app_state", ctypes.c_void_p),
        ("last_received_time", ctypes.c_float),
    ]

class JrtcApp:
    def __init__(self, env_ctx, app_cfg, app_handler, app_state):
        super().__init__()
        self.c_struct = JrtcAppCStruct(env_ctx, app_cfg, app_handler, app_state, time.monotonic())
        self.stream_items: list[StreamItem] = []

    def init(self) -> int:
        start_time = time.monotonic()
        self.last_received_time = start_time

        for i in range(self.c_struct.app_cfg.num_streams):
            stream = self.c_struct.app_cfg.streams[i]
            si = StreamItem()
            si.sid = JrtcRouterStreamId()

            ## res = jrtc_router_generate_stream_id(&sid, s.sid.destination, s.sid.device_id, s.sid.stream_source, s.sid.io_map);
            res = si.sid.generate_id(
                stream.sid.destination,
                stream.sid.device_id,
                stream.sid.stream_source,
                stream.sid.io_map,
            )
            if res != 1:
                print(f"{self.c_struct.app_cfg.context}:: Failed to generate stream ID for stream {i}")
                return -1
            _sid = si.sid
            si.sid = si.sid.convert_to_struct_jrtc_router_stream_id()

            if stream.appChannel:
                si.chan_ctx = jrtc_router_channel_create(
                    self.c_struct.env_ctx.dapp_ctx,
                    stream.appChannel.contents.is_output,
                    stream.appChannel.contents.num_elems,
                    stream.appChannel.contents.elem_size,
                    _sid,
                    None,
                    0,
                )
                if not si.chan_ctx:
                    print(f"{self.c_struct.app_cfg.context}:: Failed to create channel for stream {i}")
                    return -1

            if stream.is_rx:
                if not jrtc_router_channel_register_stream_id_req(self.c_struct.env_ctx.dapp_ctx, si.sid):
                    print(f"{self.c_struct.app_cfg.context}:: Failed to register stream {i}")
                    return -1
                si.registered = True

            self.stream_items.append(si)

            if self.c_struct.app_cfg.initialization_timeout_secs > 0:
                if time.monotonic() - start_time > self.c_struct.app_cfg.initialization_timeout_secs:
                    print(f"{self.c_struct.app_cfg.context}:: Initialization timeout")
                    return -1

        for i in range(self.c_struct.app_cfg.num_streams):
            stream = self.c_struct.app_cfg.streams[i]
            si = self.stream_items[i]
            if not stream.is_rx and not si.chan_ctx:
                k = 0
                while not jrtc_router_input_channel_exists(si.sid):
                    time.sleep(0.1)
                    if k == 10:
                        print(f"{self.c_struct.app_cfg.context}:: Waiting for creation of stream {i}")
                        k = 0
                    else:
                        k += 1

                    if self.c_struct.app_cfg.initialization_timeout_secs > 0:
                        if time.monotonic() - start_time > self.c_struct.app_cfg.initialization_timeout_secs:
                            print(f"{self.c_struct.app_cfg.context}:: Timeout waiting for stream {i}")
                            return -1
        return 0

    def cleanup(self):
        print(f"{self.c_struct.app_cfg.context}:: Cleaning up app")
        for si in self.stream_items:
            if si.registered:
                jrtc_router_channel_deregister_stream_id_req(self.c_struct.env_ctx.dapp_ctx, si.sid)
            if si.chan_ctx:
                jrtc_router_channel_destroy(si.chan_ctx)

    def run(self):        
        if self.init() != 0:
            return

        data_entries = get_data_entry_array_ptr(self.c_struct.app_cfg.q_size)
        while not self.c_struct.env_ctx.app_exit:
            now = time.monotonic()
            if (
                self.c_struct.app_cfg.inactivity_timeout_secs > 0
                and now - self.c_struct.last_received_time > self.c_struct.app_cfg.inactivity_timeout_secs
            ):
                self.c_struct.app_handler(True, -1, None, self.c_struct.app_state)
                self.last_received_time = now

            num_rcv = jrtc_router_receive(self.c_struct.env_ctx.dapp_ctx, data_entries, self.c_struct.app_cfg.q_size)
            for i in range(num_rcv):
                data_entry = data_entries[i]
                if not data_entry:
                    continue
                for sidx in range(self.c_struct.app_cfg.num_streams):
                    stream = self.c_struct.app_cfg.streams[sidx]
                    si = self.stream_items[sidx]
                    if stream.is_rx and jrtc_router_stream_id_matches_req(data_entry.stream_id, si.sid):
                        self.c_struct.app_handler(False, sidx, data_entry, self.c_struct.app_state)
                        break
                jrtc_router_channel_release_buf(data_entry.data)
                self.c_struct.last_received_time = time.monotonic()

            if self.c_struct.app_cfg.sleep_timeout_secs > 0:
                time.sleep(max(self.c_struct.app_cfg.sleep_timeout_secs, 1e-9))

        self.cleanup()

    def get_stream(self, stream_idx: int) -> Optional[JrtcRouterStreamId]:
        if stream_id < 0 or stream_idx >= len(self.stream_items):
            return
        return self.stream_items[stream_idx].sid

    def get_chan_ctx(self, stream_idx: int) -> Optional[ChannelCtx]:
        if stream_idx < 0 or stream_idx >= len(self.stream_items):
            return
        return self.stream_items[stream_idx].chan_ctx

def jrtc_app_create(capsule, app_cfg: JrtcAppCfg_t, app_handler: JrtcAppHandler, app_state):
    env_ctx = get_ctx_from_capsule(capsule)
    app_handler_c = JrtcAppHandler(app_handler)
    app_instance = JrtcApp(
        env_ctx=env_ctx, 
        app_cfg=app_cfg, 
        app_handler=app_handler_c, 
        app_state=ctypes.cast(ctypes.pointer(app_state), ctypes.c_void_p)
    )
    return app_instance

def jrtc_app_run(app) -> None:
    app.run()

def jrtc_app_destroy(app) -> None:
    app.cleanup()
    del app

def jrtc_app_router_channel_send_input_msg(app: JrtcApp, stream_idx: int, data: bytes, data_len: int) -> int:
    stream = app.get_stream(stream_idx)
    if not stream:
        return -1
    return jrtc_router_channel_send_input_msg(stream, data, data_len)

def jrtc_app_router_channel_send_output_msg(app: JrtcApp, stream_idx: int, data: bytes, data_len: int) -> int:
    chan_ctx = app.get_chan_ctx(stream_idx)
    if not chan_ctx:
        return -1
    return jrtc_router_channel_send_output_msg(chan_ctx, data, data_len)

__all__ = [
    "JRTC_ROUTER_REQ_DEST_ANY",
    "JRTC_ROUTER_REQ_DEVICE_ID_ANY",
    "JRTC_ROUTER_REQ_DEST_NONE",
    "JRTC_ROUTER_REQ_STREAM_PATH_ANY",
    "JRTC_ROUTER_REQ_STREAM_NAME_ANY",
    "struct_jrtc_router_data_entry",
    "JrtcStreamIdCfg_t",
    "JrtcAppChannelCfg_t",
    "JrtcStreamCfg_t",
    "JrtcAppCfg_t",
    "JrtcApp",
    "jrtc_app_create",
    "jrtc_app_run",
    "jrtc_app_destroy",
    "jrtc_app_router_channel_send_input_msg",
    "jrtc_app_router_channel_send_output_msg",
]
