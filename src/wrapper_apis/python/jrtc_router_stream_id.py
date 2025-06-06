# Copyright (c) Microsoft Corporation.
# Licensed under the MIT license.

import ctypes
import os
import sys
from ctypes import Structure, c_uint16, c_uint64, addressof, POINTER, sizeof
import binascii
from enum import Enum

JRTC_APP_PATH = os.environ.get("JRTC_APP_PATH")
if JRTC_APP_PATH is None:
    print("Warning: JRTC_APP_PATH not set")
    JRTC_APP_PATH = "./"
sys.path.append(f"{JRTC_APP_PATH}")

import jrtc_bindings
from jrtc_wrapper_utils import register_dll

stream_id_lib_path = os.path.join(JRTC_APP_PATH, "libjrtc_router_stream_id.so")
stream_id_lib = register_dll(stream_id_lib_path)

class JrtcRouterDest(Enum):
    JRTC_ROUTER_DEST_NONE = 0x1
    JRTC_ROUTER_DEST_UDP = 0x2
    JRTC_ROUTER_DEST_RESERVED = 0x4
    JRTC_ROUTER_DEST_RESERVED2 = 0x8
    JRTC_ROUTER_DEST_RESERVED3 = 0x10
    JRTC_ROUTER_DEST_RESERVED4 = 0x20
    JRTC_ROUTER_DEST_ANY = 0x7F


class JrtcRouterWildcards(Enum):
    JRTC_ROUTER_DEVICE_ID_ANY = 0x7F
    JRTC_ROUTER_STREAM_PATH_ANY = 0x3FFFFFFFFFFFFF
    JRTC_ROUTER_STREAM_NAME_ANY = 0x3FFFFFFFFFFFFF


class JrtcRouterReqType(Enum):
    JRTC_ROUTER_REQ_DEST_NONE = JrtcRouterDest.JRTC_ROUTER_DEST_NONE
    JRTC_ROUTER_REQ_DEST_UDP = JrtcRouterDest.JRTC_ROUTER_DEST_UDP
    JRTC_ROUTER_REQ_DEST_ANY = JrtcRouterDest.JRTC_ROUTER_DEST_ANY
    JRTC_ROUTER_REQ_DEVICE_ID_ANY = JrtcRouterWildcards.JRTC_ROUTER_DEVICE_ID_ANY
    JRTC_ROUTER_REQ_STREAM_PATH_ANY = None
    JRTC_ROUTER_REQ_STREAM_NAME_ANY = None


class JrtcRouterStreamId(Structure):
    _fields_ = [("id", ctypes.c_char * 16)]

    def __str__(self):
        return bytes(self.id).hex()

    def generate_id(self, fwd_dst, device_id, stream_path, stream_name):
        """Generates a stream ID."""
        res = stream_id_lib.jrtc_router_generate_stream_id(
            ctypes.pointer(self), fwd_dst, device_id, stream_path, stream_name
        )
        if res != 1:
            raise RuntimeError("Failed to generate stream ID {}".format(res))
        print("Generated stream ID: {}".format(self))
        return res

    def convert_to_struct_jrtc_router_stream_id(self):
        struct = jrtc_bindings.struct_jrtc_router_stream_id()
        ## copy the id
        ctypes.memmove(
            addressof(struct),
            addressof(self),
            min(sizeof(struct), sizeof(self)),
        )
        return struct

    def __str__(self):
        """Returns the hex representation of the stream ID."""
        return binascii.hexlify(
            ctypes.string_at(ctypes.byref(self), ctypes.sizeof(self))
        ).decode("utf-8")

    def to_bytearray(self):
        """Converts the structure to a bytearray."""
        return bytearray(ctypes.string_at(ctypes.byref(self), ctypes.sizeof(self)))

    def get_version(self):
        return stream_id_lib.jrtc_router_stream_id_get_ver(self)

    def get_fwd_dst(self):
        return stream_id_lib.jrtc_router_stream_id_get_fwd_dst(self)

    def get_device_id(self):
        return stream_id_lib.jrtc_router_stream_id_get_device_id(self)

    def get_stream_path(self):
        return stream_id_lib.jrtc_router_stream_id_get_stream_path(self)

    def get_stream_name(self):
        return stream_id_lib.jrtc_router_stream_id_get_stream_name(self)

    def set_version(self, version):
        stream_id_lib.jrtc_router_stream_id_set_ver(self, version)

    def set_fwd_dst(self, dst):
        stream_id_lib.jrtc_router_stream_id_set_fwd_dst(self, dst)

    def set_device_id(self, device_id):
        stream_id_lib.jrtc_router_stream_id_set_device_id(self, device_id)

    def set_stream_path(self, stream_path):
        b_stream_path = stream_path.encode("utf-8")
        stream_id_lib.jrtc_router_stream_id_set_stream_path(
            self, ctypes.c_char_p(stream_path)
        )

    def set_stream_name(self, stream_name):
        b_stream_name = stream_name.encode("utf-8")
        stream_id_lib.jrtc_router_stream_id_set_stream_name(
            self, ctypes.c_char_p(stream_name)
        )

def jrtc_router_stream_id_matches_req(a, b):
    stream_id_lib.__jrtc_router_stream_id_matches_req.argtypes = [
        ctypes.POINTER(jrtc_bindings.struct_jrtc_router_stream_id),
        ctypes.POINTER(jrtc_bindings.struct_jrtc_router_stream_id),
    ]
    stream_id_lib.__jrtc_router_stream_id_matches_req.restype = ctypes.c_bool
    return stream_id_lib.__jrtc_router_stream_id_matches_req(a, b)


def jrtc_router_stream_id_get_device_id(s):
    stream_id_lib.__jrtc_router_stream_id_get_device_id.argtypes = [
        ctypes.POINTER(jrtc_bindings.struct_jrtc_router_stream_id),
    ]
    stream_id_lib.__jrtc_router_stream_id_get_device_id.restype = ctypes.c_uint16
    return stream_id_lib.__jrtc_router_stream_id_get_device_id(s)


stream_id_lib.jrtc_router_generate_stream_id.argtypes = [
    ctypes.POINTER(JrtcRouterStreamId),
    ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_char_p
]
stream_id_lib.jrtc_router_generate_stream_id.restype = ctypes.c_int
