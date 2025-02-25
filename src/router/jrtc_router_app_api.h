// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_ROUTER_APP_API_H
#define JRTC_ROUTER_APP_API_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "jbpf_io_channel.h"
#include "jrtc_router_stream_id.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct dapp_router_ctx* dapp_router_ctx_t;

    typedef struct dapp_channel_ctx* dapp_channel_ctx_t;

    /**
     * @brief The jrtc_router_data_entry struct
     * @ingroup router
     * stream_id: The stream id
     * data: The data
     */
    typedef struct jrtc_router_data_entry
    {
        struct jrtc_router_stream_id stream_id;
        void* data;
    } jrtc_router_data_entry_t;

    /// @brief Registers an app to the jrtc router.
    /// @ingroup router
    /// @param app_queue_size The queue size used for storing incoming messages from the subscribed channels of this
    /// application
    /// @return A handler to the router context if successful or NULL otherewise.
    dapp_router_ctx_t
    jrtc_router_register_app(size_t app_queue_size);

    /// @brief Deregisters an app from the router
    /// @ingroup router
    /// @param app_ctx The context of the app.
    void
    jrtc_router_deregister_app(dapp_router_ctx_t app_ctx);

    /// @brief Can be used by jrt-controller applications to request to receive data from one or more channels.
    /// The request can identify a specific stream id or can use wildcards to capture data of several channels.
    /// The request is translated into a stream_id and is passed to jrtc_router_channel_register_stream_id().
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param fwd_dst Destination endpoint(s) for channel data, if forwarding over the network.
    /// Can be a logical or of the JRTC_ROUTER_DEST_* values or JRTC_ROUTER_DEST_ANY for wildcard.
    /// @param device_id The unique identifier of the device of origin for this stream (e.g., jbpf or eBPF agent,
    /// jrt-controller, etc.). Can be a value between 0 and 126 or JRTC_ROUTER_DEVICE_ID_ANY for wildcard.
    /// @param stream_path Identifies the location of origin of this stream (e.g. "codeletset_id/codelet_name").
    /// NULL corresponds to wildcard.
    /// @param stream_name A unique stream name under the stream path (e.g. "jbpf_output_map_name").
    /// NULL corresponds to wildcard.
    /// @return 1 if the request was successful or negative value otherwise.
    int
    jrtc_router_channel_register_req(
        dapp_router_ctx_t app_ctx, int fwd_dst, int device_id, const char* stream_path, const char* stream_name);

    /// @brief Can be used by jrt-controller applications to request to receive data from one or more channels.
    /// The request can identify a specific stream id or can use wildcards to capture data of several channels.
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param stream_id Can be an exact stream_id or can have some of its fields replaced with wildcards, to receive
    /// data from several channels.
    /// @return 1 if the request was successful or negative value otherwise.
    int
    jrtc_router_channel_register_stream_id_req(dapp_router_ctx_t app_ctx, struct jrtc_router_stream_id stream_id);

    /// @brief Unsubscribes an app from a channel subscription request, if the request exists.
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param fwd_dst Destination endpoint(s) for channel data, if forwarding over the network.
    /// @param device_id The unique identifier of the device of origin for this stream.
    /// @param stream_path Identifies the location of origin of this stream
    /// @param stream_name A unique stream name under the stream path
    void
    jrtc_router_channel_deregister_req(
        dapp_router_ctx_t app_ctx, int fwd_dst, int device_id, const char* stream_path, const char* stream_name);

    /// @brief Unsubscribes an app from a channel subscription request, if the request exists
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param stream_id The stream id that was used when calling jrtc_router_channel_register_stream_id().
    void
    jrtc_router_channel_deregister_stream_id_req(dapp_router_ctx_t app_ctx, struct jrtc_router_stream_id stream_id);

    /// @brief Populates an array of jrtc_router_data_entry_t entries, with data arriving from subscribed channels.
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param data_entries An array of jrtc_router_data_entry_t entries to be filled by the callee.
    /// @param num_entries The size of data_entries.
    /// @return The number of received entries (0 up to num_entries) if successful, or a negative value in the case of
    /// an error.
    int
    jrtc_router_receive(dapp_router_ctx_t app_ctx, jrtc_router_data_entry_t* data_entries, size_t num_entries);

    /// @brief Reserves a buffer from a channel allocated by the caller app.
    /// Is used in conjunction with jrtc_router_channel_send_output().
    /// @ingroup router
    /// @param chan_ctx The context of the channel that will transport the data.
    /// @return A pointer to the reserved buffer or NULL if a buffer could not be allocated.
    void*
    jrtc_router_channel_reserve_buf(dapp_channel_ctx_t chan_ctx);

    /// @brief Sends some output data to a channel created by the caller application.
    /// The call does not require a pointer to a data buffer as a parameter.
    /// Instead it sends any data that has been copied in the data buf exposed to the app when calling
    /// jrtc_router_channel_reserve_buf() .
    /// @ingroup router
    /// @param chan_ctx The context of the channel that will transport the data.
    /// @return 0 if the send was successful or a negative number otherwise.
    int
    jrtc_router_channel_send_output(dapp_channel_ctx_t chan_ctx);

    /// @brief Sends some output data to an output channel.  It is the same as "jrtc_router_channel_send_output" except
    //  that the data is passed and copied in the function.
    /// @ingroup router
    /// @param chan_ctx The context of the channel that will transport the data.
    /// @param data A pointer to some data to be sent over the channel. It is expected that the consumer knows how to
    /// parse the received data.
    /// @param data_len The size of the data buffer.
    /// @return 0 if the send was successful or a negative number otherwise.
    int
    jrtc_router_channel_send_output_msg(dapp_channel_ctx_t chan_ctx, void* data, size_t data_len);

    /// @brief Sends some data (e.g., control input) to a channel created by another app or agent code
    /// (e.g., jbpf codelet or eBPF agent).
    /// @ingroup router
    /// @param stream_id The stream id of the target channel. Can be an exact stream_id or can have some of its fields
    /// replaced with wildcards (e.g., to send the same data to several codelets loaded at different agents).
    /// @param data A pointer to some data to be sent over the channel. It is expected that the consumer knows how to
    /// parse the received data.
    /// @param data_len The size of the data buffer.
    /// @return 0 if the send was successful or a negative number otherwise.
    int
    jrtc_router_channel_send_input_msg(struct jrtc_router_stream_id stream_id, void* data, size_t data_len);

    /// @brief Releases a buffer allocated with jrtc_router_channel_reserve_buf().
    /// Typically this function is expected to be called by the consumer of the data.
    /// @ingroup router
    /// @param ptr The pointer to the allocated buffer.
    void
    jrtc_router_channel_release_buf(void* ptr);

    /// @brief Creates an input or output channel to be used by the caller app.
    /// @ingroup router
    /// @param app_ctx The context of the app.
    /// @param is_output true if this will be an output channel or false otherwise.
    /// @param num_elems The number of elements of this channel.
    /// @param elem_size The size of the elements.
    /// @param stream_id The stream id of the target channel. This stream id cannot contain any wildcards.
    /// @param descriptor A descriptor for the encoding/decoding of the data carried by the channel. This is
    /// a byte array of a dynamic library generated using nanopb. Can be NULL, if this channel is not to be used
    /// to send data over the network.
    /// @param descriptor_size Size of the descriptor. Can be 0, if the channel does not have a descriptor.
    /// @return
    dapp_channel_ctx_t
    jrtc_router_channel_create(
        dapp_router_ctx_t app_ctx,
        bool is_output,
        int num_elems,
        int elem_size,
        jrtc_router_stream_id_t stream_id,
        char* descriptor,
        size_t descriptor_size);

    /// @brief Destroys a channel created by the caller app.
    /// @ingroup router
    /// @param dapp_chan_ctx The context of the channel to be destroyed.
    void
    jrtc_router_channel_destroy(dapp_channel_ctx_t dapp_chan_ctx);

    /// @brief Checks if an input channel exists (e.g., before actually sending data to it)
    /// @ingroup router
    /// @param stream_id The stream id of the target channel to check.
    int
    jrtc_router_input_channel_exists(struct jrtc_router_stream_id stream_id);

    /// @brief Creates a serialized message that is ready to be sent out over the network.
    /// @ingroup router
    /// @param data_entry A data entry of the data to be encoded.
    /// @param buf A buffer to store the outgoing jrt-controller io message.
    /// @param buf_len The size of the buffer.
    /// @return The amount of data stored in buf if the data was encoded successfully
    /// and a negative value otherwise (e.g., if /// the buffer is too small,
    /// the channel has no encoder descriptor, etc.).
    int
    jrtc_router_create_serialized_msg(jrtc_router_data_entry_t data_entry, void* buf, size_t buf_len);

    /// @brief Deserializes some data received over the network for a particular channel.
    /// @ingroup router
    /// @param in_data A buffer with the received serialized data.
    /// @param in_data_size The size of the in_data buffer.
    /// @param stream_id Stores the stream id of the channel the data is intended for.
    /// @param req_id Stores the request id of the received message.
    /// @return jbpf_channel_buf_ptr Returns a pointer to a buffer of type jpbf_channel_buf_ptr, that contains the
    /// deserialized message.
    jbpf_channel_buf_ptr
    jrtc_router_deserialize_msg(
        void* in_data, size_t in_data_size, struct jrtc_router_stream_id* stream_id, int* req_id);

#ifdef __cplusplus
}
#endif

#endif