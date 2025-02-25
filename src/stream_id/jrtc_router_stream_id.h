// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_ROUTER_STREAM_ID_
#define JRTC_ROUTER_STREAM_ID_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "jrtc_router_bitmap.h"
#include "jbpf_io_channel_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define JRTC_ROUTER_CONTROLLER_DEVICE_ID 0

// For *_ANY fields, all 54 bits are set to 1
// Use 54 bits and 38 hash functions for the path fields

/**
 * @brief The number of bits used for the hash number
 * @ingroup stream_id
 * The number of bits used for the hash number
 */
#define JRTC_ROUTER_HASH_NUMBER_BITS (54)

/**
 * @brief The number of hash functions
 * @ingroup stream_id
 * The number of hash functions
 */
#define JRTC_ROUTER_NUM_HASH_FUNCTIONS (38)

#define JRTC_ROUTER_STREAM_ID_VERSION (0)

/**
 * @brief This value is used to indicate that the destination is not set
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_NONE (0x1)
// UDP output
/**
 * @brief This value is used to indicate that the destination is a UDP port
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_UDP (0x2)
/**
 * @brief Reserved for future use
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_RESERVED (0x4)
/**
 * @brief Reserved for future use
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_RESERVED2 (0x8)
/**
 * @brief Reserved for future use
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_RESERVED3 (0x10)
/**
 * @brief Reserved for future use
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_RESERVED4 (0x20)
/**
 * @brief Reserved for future use
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEST_ANY (0x7F)

/**
 * @brief This value is used to indicate that the device id can be any
 * @ingroup stream_id
 */
#define JRTC_ROUTER_DEVICE_ID_ANY (0x7F)

/**
 * @brief This value is used to indicate that the stream path can be any
 * @ingroup stream_id
 */
#define JRTC_ROUTER_STREAM_PATH_ANY (0x3FFFFFFFFFFFFF)

/**
 * @brief This value is used to indicate that the stream name can be any
 * @ingroup stream_id
 */
#define JRTC_ROUTER_STREAM_NAME_ANY (0x3FFFFFFFFFFFFF)

#define JRTC_ROUTER_REQ_DEST_NONE JRTC_ROUTER_DEST_NONE
#define JRTC_ROUTER_REQ_DEST_UDP JRTC_ROUTER_DEST_UDP
#define JRTC_ROUTER_REQ_DEST_ANY JRTC_ROUTER_DEST_ANY
#define JRTC_ROUTER_REQ_DEVICE_ID_ANY JRTC_ROUTER_DEVICE_ID_ANY
#define JRTC_ROUTER_REQ_STREAM_PATH_ANY (NULL)
#define JRTC_ROUTER_REQ_STREAM_NAME_ANY (NULL)

#define JRTC_ROUTER_STREAM_ID_BYTE_LEN JBPF_IO_STREAM_ID_LEN

    /* Internal representation of stream id. Should not be used to generate stream_ids directly*/
    struct jrtc_router_stream_id_int
    {
        uint16_t ver : 6;
        uint16_t fwd_dst : 7;
        uint16_t device_id : 7;
        uint64_t stream_path : 54;
        uint64_t stream_name : 54;
    };

#define _jrtc_router_stream_id_apply_ver_mask(stream_id, ver) stream_id[0] |= (ver << 2) | (stream_id[0] & 0x03);

#define _jrtc_router_stream_id_apply_fwd_dst_mask(stream_id, fwd_dst) \
    stream_id[0] |= (stream_id[0] & 0xFC) | ((fwd_dst & 0x7F) >> 5);  \
    stream_id[1] |= ((fwd_dst & 0x1F) << 3) | (stream_id[1] & 0x07);

#define _jrtc_router_stream_id_apply_device_id_mask(stream_id, device_id) \
    stream_id[1] |= (stream_id[1] & 0xF8) | ((device_id >> 4) & 0x07);    \
    stream_id[2] |= ((device_id & 0x0F) << 4) | (stream_id[2] & 0x0F);

#define _jrtc_router_stream_id_apply_stream_path_mask(stream_id, stream_path) \
    stream_id[2] |= (stream_id[2] & 0xF0) | ((stream_path >> 50) & 0x0F);     \
    stream_id[3] |= (stream_path >> 42) & 0xFF;                               \
    stream_id[4] |= (stream_path >> 34) & 0xFF;                               \
    stream_id[5] |= (stream_path >> 26) & 0xFF;                               \
    stream_id[6] |= (stream_path >> 18) & 0xFF;                               \
    stream_id[7] |= (stream_path >> 10) & 0xFF;                               \
    stream_id[8] |= (stream_path >> 2) & 0xFF;                                \
    stream_id[9] |= ((stream_path & 0x03) << 6) | (stream_id[9] & 0x3F);

#define _jrtc_router_stream_id_apply_stream_name_mask(stream_id, stream_name) \
    stream_id[9] |= (stream_id[9] & 0xC0) | ((stream_name >> 48) & 0x3F);     \
    stream_id[10] |= (stream_name >> 40) & 0xFF;                              \
    stream_id[11] |= (stream_name >> 32) & 0xFF;                              \
    stream_id[12] |= (stream_name >> 24) & 0xFF;                              \
    stream_id[13] |= (stream_name >> 16) & 0xFF;                              \
    stream_id[14] |= (stream_name >> 8) & 0xFF;                               \
    stream_id[15] |= stream_name & 0xFF;

#define _jrtc_router_stream_id_set_ver(stream_id, ver) stream_id[0] = (ver << 2) | (stream_id[0] & 0x03);

#define _jrtc_router_stream_id_set_fwd_dst(stream_id, fwd_dst)      \
    stream_id[0] = (stream_id[0] & 0xFC) | ((fwd_dst & 0x7F) >> 5); \
    stream_id[1] = ((fwd_dst & 0x1F) << 3) | (stream_id[1] & 0x07);

#define _jrtc_router_stream_id_set_device_id(stream_id, device_id)    \
    stream_id[1] = (stream_id[1] & 0xF8) | ((device_id >> 4) & 0x07); \
    stream_id[2] = ((device_id & 0x0F) << 4) | (stream_id[2] & 0x0F);

#define _jrtc_router_stream_id_set_stream_path(stream_id, stream_path)   \
    stream_id[2] = (stream_id[2] & 0xF0) | ((stream_path >> 50) & 0x0F); \
    stream_id[3] = (stream_path >> 42) & 0xFF;                           \
    stream_id[4] = (stream_path >> 34) & 0xFF;                           \
    stream_id[5] = (stream_path >> 26) & 0xFF;                           \
    stream_id[6] = (stream_path >> 18) & 0xFF;                           \
    stream_id[7] = (stream_path >> 10) & 0xFF;                           \
    stream_id[8] = (stream_path >> 2) & 0xFF;                            \
    stream_id[9] = ((stream_path & 0x03) << 6) | (stream_id[9] & 0x3F);

#define _jrtc_router_stream_id_set_stream_name(stream_id, stream_name)   \
    stream_id[9] = (stream_id[9] & 0xC0) | ((stream_name >> 48) & 0x3F); \
    stream_id[10] = (stream_name >> 40) & 0xFF;                          \
    stream_id[11] = (stream_name >> 32) & 0xFF;                          \
    stream_id[12] = (stream_name >> 24) & 0xFF;                          \
    stream_id[13] = (stream_name >> 16) & 0xFF;                          \
    stream_id[14] = (stream_name >> 8) & 0xFF;                           \
    stream_id[15] = stream_name & 0xFF;

#define _jrtc_router_stream_id_get_ver(stream_id) ((stream_id[0] >> 2) & 0x3F);

#define _jrtc_router_stream_id_get_fwd_dst(stream_id) (((stream_id[0] & 0x03) << 5) | ((stream_id[1] >> 3) & 0x1F));

#define _jrtc_router_stream_id_get_device_id(stream_id) (((stream_id[1] & 0x07) << 4) | ((stream_id[2] >> 4) & 0x0F));

#define _jrtc_router_stream_id_get_stream_path(stream_id)                                                         \
    ((((uint64_t)stream_id[2] & 0x0FL) << 50) | ((uint64_t)stream_id[3] << 42) | ((uint64_t)stream_id[4] << 34) | \
     ((uint64_t)stream_id[5] << 26) | ((uint64_t)stream_id[6] << 18) | ((uint64_t)stream_id[7] << 10) |           \
     ((uint64_t)stream_id[8] << 2) | (((uint64_t)stream_id[9] & 0xC0L) >> 6));

#define _jrtc_router_stream_id_get_stream_name(stream_id)                                                          \
    ((((uint64_t)stream_id[9] & 0x3F) << 48) | ((uint64_t)stream_id[10] << 40) | ((uint64_t)stream_id[11] << 32) | \
     ((uint64_t)stream_id[12] << 24) | ((uint64_t)stream_id[13] << 16) | ((uint64_t)stream_id[14] << 8) |          \
     (stream_id[15]));

    /**
     * @brief The jrtc_router_stream_id struct
     * @ingroup stream_id
     * id: The stream id
     */
    typedef struct jrtc_router_stream_id
    {
        char id[JRTC_ROUTER_STREAM_ID_BYTE_LEN];
    } jrtc_router_stream_id_t;

    /**
     * @brief Apply the version mask to the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param ver The version
     */
    void
    jrtc_router_stream_id_set_ver(jrtc_router_stream_id_t* stream_id, uint16_t ver);

    /**
     * @brief Apply the forward destination mask to the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param fwd_dst The forward destination
     */
    void
    jrtc_router_stream_id_set_fwd_dst(jrtc_router_stream_id_t* stream_id, uint16_t fwd_dst);

    /**
     * @brief Apply the device id mask to the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param device_id The device id
     */
    void
    jrtc_router_stream_id_set_device_id(jrtc_router_stream_id_t* stream_id, uint16_t device_id);

    /**
     * @brief Apply the stream path mask to the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param stream_path The stream path
     */
    void
    jrtc_router_stream_id_set_stream_path(jrtc_router_stream_id_t* stream_id, uint64_t stream_path);

    /**
     * @brief Apply the stream name mask to the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param stream_name The stream name
     */
    void
    jrtc_router_stream_id_set_stream_name(jrtc_router_stream_id_t* stream_id, uint64_t stream_name);

    /**
     * @brief Get the version from the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @return The version
     */
    uint16_t
    jrtc_router_stream_id_get_ver(const jrtc_router_stream_id_t* stream_id);

    /**
     * @brief Get the forward destination from the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @return The forward destination
     */
    uint16_t
    jrtc_router_stream_id_get_fwd_dst(const jrtc_router_stream_id_t* stream_id);

    /**
     * @brief Get the device id from the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @return The device id
     */
    uint16_t
    jrtc_router_stream_id_get_device_id(const jrtc_router_stream_id_t* stream_id);

    /**
     * @brief Get the stream path from the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @return The stream path
     */
    uint64_t
    jrtc_router_stream_id_get_stream_path(const jrtc_router_stream_id_t* stream_id);

    /**
     * @brief Get the stream name from the stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @return The stream name
     */
    uint64_t
    jrtc_router_stream_id_get_stream_name(const jrtc_router_stream_id_t* stream_id);

    /**
     * @brief Generate a stream id
     * @ingroup stream_id
     * @param stream_id The stream id
     * @param fwd_dst The forward destination
     * @param device_id The device id
     * @param stream_path The stream path
     * @param stream_name The stream name
     * @return 1 on success, -1 on failure
     */
    int
    jrtc_router_generate_stream_id(
        jrtc_router_stream_id_t* stream_id,
        int fwd_dst,
        int device_id,
        const char* stream_path,
        const char* stream_name);

    /**
     * @brief Generate a stream id from a stream id request
     * @ingroup stream_id
     * @param sid The stream id
     * @param sid_req The stream id request
     * @return true if the stream id matches the request, false otherwise
     */
    static inline bool
    jrtc_router_stream_id_matches_req(const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
    {

        for (int j = 0; j < 4; j++) {
            if ((((int*)sid->id)[j] & ((int*)sid_req->id)[j]) != ((int*)sid->id)[j])
                return false;
        }

        return true;
    }

    /**
     * @brief Check if the forward destination of the stream id matches the request
     * @ingroup stream_id
     * @param sid The stream id
     * @param sid_req The stream id request
     * @return true if the forward destination matches the request, false otherwise
     */
    static inline bool
    jrtc_router_stream_id_fwd_dst_matches_req(
        const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
    {
        return (((struct jrtc_router_stream_id_int*)sid)->fwd_dst &
                ((struct jrtc_router_stream_id_int*)sid_req)->fwd_dst) ==
               ((struct jrtc_router_stream_id_int*)sid)->fwd_dst;
    }

    /**
     * @brief Check if the device id of the stream id matches the request
     * @ingroup stream_id
     * @param sid The stream id request
     * @param sid_req The stream id request
     * @return true if the device id matches the request, false otherwise
     */
    static inline bool
    jrtc_router_stream_id_device_id_matches_req(
        const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
    {
        return (((struct jrtc_router_stream_id_int*)sid)->device_id &
                ((struct jrtc_router_stream_id_int*)sid_req)->device_id) ==
               ((struct jrtc_router_stream_id_int*)sid)->device_id;
    }

    /**
     * @brief Check if the stream path of the stream id matches the request
     * @ingroup stream_id
     * @param sid The stream id
     * @param sid_req The stream id request
     * @return true if the stream path matches the request, false otherwise
     */
    static inline bool
    jrtc_router_stream_id_stream_path_matches_req(
        const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
    {
        return (((struct jrtc_router_stream_id_int*)sid)->stream_path &
                ((struct jrtc_router_stream_id_int*)sid_req)->stream_path) ==
               ((struct jrtc_router_stream_id_int*)sid)->stream_path;
    }

    /**
     * @brief Check if the stream name of the stream id matches the request
     * @ingroup stream_id
     * @param sid The stream id
     * @param sid_req The stream id request
     * @return true if the stream name matches the request, false otherwise
     */
    static inline bool
    jrtc_router_stream_id_stream_name_matches_req(
        const jrtc_router_stream_id_t* sid, const jrtc_router_stream_id_t* sid_req)
    {
        return (((struct jrtc_router_stream_id_int*)sid)->stream_name &
                ((struct jrtc_router_stream_id_int*)sid_req)->stream_name) ==
               ((struct jrtc_router_stream_id_int*)sid)->stream_name;
    }

#ifdef __cplusplus
}
#endif

#endif