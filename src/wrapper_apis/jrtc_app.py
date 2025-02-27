# Copyright (c) Microsoft Corporation. All rights reserved.

import time
import os
import sys
import ctypes
from dataclasses import dataclass
from typing import List, Callable, Optional, Any

# Retrieve the JRTC application path from environment variables
JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")

sys.path.append(f"{JRTC_APP_PATH}/out/lib")
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


########################################################
# Data classes representing configuration structures
########################################################

@dataclass
class StreamCfg:
    """
    Represents a Stream ID configuration containing routing details.
    """
    destination: int
    device_id: int
    stream_source: str
    io_map: str

    def __str__(self):
        return (f"StreamCfg(destination={self.destination}, "
                f"device_id={self.device_id}, "
                f"stream_source='{self.stream_source}', "
                f"io_map='{self.io_map}')")


@dataclass
class StreamIdCfg:
    destination: int
    device_id: int
    stream_source: str
    io_map: str

    def __str__(self):
        return (f"StreamIdCfg(destination={self.destination}, "
                f"device_id={self.device_id}, "
                f"stream_source={self.stream_source}, "
                f"io_map={self.io_map}")

@dataclass
class AppChannelCfg():
    """
    Represents an application channel configuration.
    """
    is_output: bool   # Whether the channel is output
    num_elems: int    # Number of elements in the channel
    elem_size: int    # Size of each element

    def __str__(self):
        return (f"AppChannelCfg(is_output={self.is_output}, "
                f"num_elems={self.num_elems}, "
                f"elem_size={self.elem_size})")
@dataclass
class StreamCfg:
    """
    Represents a stream configuration, including StreamId and optional AppChannel.
    """
    sid: StreamIdCfg
    is_rx: bool  # Whether this channel is read from or sent to
    appChannel: Optional[AppChannelCfg] = None

    def __str__(self):
        return (f"StreamCfg(sid={self.sid}, "
                f"is_rx={self.is_rx}, "
                f"appChannel={self.appChannel})")

@dataclass
class AppCfg:
    """
    Represents the main application configuration.
    """
    context: str  # Application context name
    q_size: int   # Queue size for messages
    streams: List[StreamCfg]  # List of stream configurations
    sleep_timeout_secs: float  # Sleep timeout for the main loop
    inactivity_timeout_secs: float  # Inactivity timeout before triggering a timeout event

    def __str__(self):
        streams_str = "\n  ".join(str(s) for s in self.streams)
        return (f"AppCfg(\n"
                f"  context='{self.context}',\n"
                f"  q_size={self.q_size},\n"
                f"  streams=[\n  {streams_str}\n  ],\n"
                f"  sleep_timeout_secs={self.sleep_timeout_secs},\n"
                f"  inactivity_timeout_secs={self.inactivity_timeout_secs}\n"
                f")")


########################################################
@dataclass
class StreamItem:
    """
    Represents a stream item containing references to a stream ID, request ID, and channel context.
    """
    stream_id: JrtcRouterStreamId
    req_id: struct_jrtc_router_stream_id
    chan_ctx: struct_dapp_channel_ctx


########################################################
class JrtcPythonApp():
    """
    A wrapper for Python applications interacting with JRTC.
    Manages stream creation, message handling, and cleanup.
    """

    ############################################
    def __init__(self, capsule, app_cfg: AppCfg, app_handler: Callable[[bool, int, struct_jrtc_router_data_entry, Any], None], app_state: Any):
        self.capsule = capsule
        self.app_cfg = app_cfg
        self.app_handler = app_handler
        self.app_state = app_state
        self.stream_items = [None] * len(self.app_cfg.streams)

    ############################################
    def Init(self) -> None:
        """
        Initializes the application by creating streams and registering them if needed.
        """

        self.env_ctx = get_ctx_from_capsule(self.capsule)
        self.app_ctx = self.env_ctx.dapp_ctx.contents    

        print(f"{self.app_cfg.context}::  App started with ctx: {self.env_ctx}!")
        
        print(self.app_cfg)

        self.last_received_time = time.time() 
        
        # create all streams
        for idx, s in enumerate(self.app_cfg.streams):
            
            stream_id = JrtcRouterStreamId()

            # generate stream_id
            res = stream_id.generate_id(s.sid.destination, s.sid.device_id, s.sid.stream_source, s.sid.io_map)
            if res == 1:
                print(f"{self.app_cfg.context}::  Stream id successfully generated for '{s.sid}'")
            else:
                assert False, f"{self.app_cfg.context}::  ERROR: Stream id could not be generated for '{s.sid}'"

            si = StreamItem(stream_id=stream_id, req_id=None, chan_ctx=None)

            if s.appChannel:
                print(f"Creating appChannel for {s.sid}")
                chan_ctx = jrtc_router_channel_create(self.app_ctx, 
                    s.appChannel.is_output, s.appChannel.num_elems, s.appChannel.elem_size,
                    stream_id, 0, 0)
                assert chan_ctx is not None, f"{self.app_cfg.context}::  Failure to create channel for stream idx {idx}"
                si.chan_ctx = chan_ctx
                    
            # if is_rx, register the stream
            req_id = stream_id.convert_to_struct_jrtc_router_stream_id()
            if s.is_rx:
                # it is a channel which this app will be read from, so register
                res = jrtc_router_channel_register_stream_id_req(self.app_ctx, req_id)
                if res == 1:
                    print(f"{self.app_cfg.context}::  Stream id successfully registered for '{s.sid}'")
                    si.req_id = req_id
                else:
                    assert False, f"{self.app_cfg.context}::  ERROR: Stream id could not be registered for '{s.sid}'"
            
            self.stream_items[idx] = si

        # wait for channels
        # if not is_rx, wait for channel to be created
        for idx, s in enumerate(self.app_cfg.streams):
            si = self.stream_items[idx]
            if (s.is_rx is False) and (si is not None):
                # it is a channel which this app will be send to, so wait for it be created
                req_id = si.stream_id.convert_to_struct_jrtc_router_stream_id()
                write_notif = True
                print(f"Waiting for {s.sid} to be created...")
                while not jrtc_router_input_channel_exists(req_id):
                    
                    time.sleep(0.1)
                    
                    if write_notif:
                        print(f"Waiting for {s.sid} to be created...")
                        write_notif = False
                si.req_id = req_id            

    ############################################
    def CleanUp(self) -> None:
        """
        Cleans up the application by deleting streams and de-registering them if needed.
        """

        for si in self.stream_items:

            if si.req_id is not None:
                jrtc_router_channel_deregister_stream_id_req(self.app_ctx, si.req_id)

            if si.chan_ctx is not None:
                jrtc_router_channel_destroy(si.chan_ctx)

    ############################################
    def run(self) -> None:
        """
        Runs the main loop for handling messages.
        """

        self.Init()

        # run the main loop

        data_entries_array_ptr = get_data_entry_array_ptr(self.app_cfg.q_size)
        
        # loop until app exits
        while not self.env_ctx.app_exit:

            # Check for timeout: If inactivity linit exceeded, call the app_handler
            if time.time() - self.last_received_time > self.app_cfg.inactivity_timeout_secs:
                self.app_handler(timeout=True, stream_idx=-1, data_entry=None, state=self.app_state)
                self.last_received_time = time.time()  # Reset to avoid repetitive timeouts

            # pool for received data
            num_rcv = jrtc_router_receive(ctypes.pointer(self.app_ctx), data_entries_array_ptr, self.app_cfg.q_size)

            # loop and process all receoved messages
            for i in range(num_rcv):

                data_entry = data_entries_array_ptr[i]

                # get stream of received data    
                rx_stream_id = data_entry.stream_id

                # does this match a stream
                stream_idx = next((i for i, item in enumerate(self.stream_items) if ((item is not None) and (jrtc_router_stream_id_matches_req(rx_stream_id, item.req_id)))), -1)

                if stream_idx == -1:
                    # no match
                    continue
                    
                self.app_handler(timeout=False, stream_idx=stream_idx, data_entry=data_entry, state=self.app_state)

                self.last_received_time = time.time()  # Reset the timeout timer

            time.sleep(self.app_cfg.sleep_timeout_secs)

        self.CleanUp()

    ######################################################
    def get_stream(self, stream_idx: int) -> struct_jrtc_router_stream_id:
        """
        Given a stream _index, return the Jrtc streamId, if present.
        """

        if stream_idx >= len(self.stream_items):
            return None

        si = self.stream_items[stream_idx]

        if si is None:
            return None

        if si.req_id is None:
            return None

        return si.req_id

    ######################################################
    def get_chan_ctx(self, stream_idx: int) -> struct_dapp_channel_ctx:
        """
        Given a stream _index, return the Jrtc channel context, if present.
        """

        if stream_idx >= len(self.stream_items):
            return None

        si = self.stream_items[stream_idx]

        if si is None:
            return None
 
        return si.chan_ctx


######################################################
def jrtc_app_get_stream(wrapper, stream_idx: int) -> struct_jrtc_router_stream_id:
    """
    Wrapper to call JrtcPythonApp.get_stream
    """
    return wrapper.get_stream(stream_idx)


######################################################
def jrtc_app_get_channel_context(wrapper, stream_idx: int) -> struct_dapp_channel_ctx:
    """
    Wrapper to call JrtcPythonApp.get_chan_ctx
    """
    return wrapper.get_chan_ctx(stream_idx)


########################################################
# Lists of items to expose when this module is importd
########################################################
__all__ = [ 
    'JRTC_ROUTER_REQ_DEST_ANY',
    'JRTC_ROUTER_REQ_DEVICE_ID_ANY',
    'JRTC_ROUTER_REQ_DEST_NONE',
    'JRTC_ROUTER_REQ_STREAM_PATH_ANY',
    'JRTC_ROUTER_REQ_STREAM_NAME_ANY',
    'JrtcPythonApp',
    'struct_jrtc_router_data_entry',
    'jrtc_app_get_stream', 
    'jrtc_app_get_channel_context',
    'jrtc_router_channel_send_input_msg',
    'jrtc_router_channel_send_output_msg',
    'AppCfg', 
    'StreamCfg', 
    'StreamIdCfg', 
    'AppChannelCfg'
]
