// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdatomic.h>

#include "jrtc.h"
#include "jrtc_router.h"
#include "jrtc_router_app_api.h"
#include "jrtc_router_stream_id.h"

#include "jrtc_logging.h"
#include "jbpf_io_utils.h"
#include "jbpf_mempool.h"

#define AA_OUT_BUFS_BATCH_SIZE (100)
#define AA_NET_OUT_DEFAULT_PORT (20788)
#define AA_NET_IN_DEFAULT_PORT (1924)
#define AA_NET_MAX_FQDN_SIZE (256)
#define AA_NET_OUT_DEFAULT_FQDN "127.0.0.1"
#define AA_NET_HEADROOM (128)
#define AA_NET_ELEM_SIZE (65535 - AA_NET_HEADROOM)
#define AA_NET_NUM_DATA_ELEM (4095U)

/* Compiler magic to make address sanitizer ignore
   memory leaks originating from libpython */
__attribute__((used)) const char*
__asan_default_options()
{
    return "detect_odr_violation=0:intercept_tls_get_addr=0:suppressions=suppress_python";
}

__attribute__((used)) const char*
__lsan_default_options()
{
    return "print_suppressions=0";
}

__attribute__((used)) const char*
__lsan_default_suppressions()
{
    return "leak:libpython";
}

#define jrtc_print_stream_id(msg, X)                                                                         \
    char sname[JRTC_ROUTER_STREAM_ID_BYTE_LEN * 3];                                                          \
    _jbpf_io_tohex_str((char*)X, JRTC_ROUTER_STREAM_ID_BYTE_LEN, sname, JRTC_ROUTER_STREAM_ID_BYTE_LEN * 3); \
    printf(msg, sname);

struct aa_net_info
{
    int in_sockfd;
    int out_sockfd;
    struct jbpf_mempool* net_mempool;
    struct sockaddr_in collector_addr;
    struct sockaddr_in control_input_addr;
    char fqdn[AA_NET_MAX_FQDN_SIZE];
};

static inline int
equal_addr(struct sockaddr_in* sa1, struct sockaddr_in* sa2)
{
    return (sa1->sin_addr.s_addr == sa2->sin_addr.s_addr);
}

int
aa_net_update_fqdn(struct aa_net_info* net_info)
{
    struct addrinfo hints, *res;
    struct sockaddr_in tmp_addr;
    int status;

    /* Check if we have IP */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if ((status = getaddrinfo(net_info->fqdn, NULL, &hints, &res)) != 0) {
        jrtc_logger(JRTC_ERROR, "FQDN resolution failure: %s (%s)\n", gai_strerror(status), net_info->fqdn);
        return -1;
    }

    tmp_addr = *((struct sockaddr_in*)res->ai_addr);
    freeaddrinfo(res);

    if (!equal_addr(&net_info->collector_addr, &tmp_addr)) {
        char ipstr[INET6_ADDRSTRLEN];
        (void)__sync_lock_test_and_set(&net_info->collector_addr.sin_addr.s_addr, tmp_addr.sin_addr.s_addr);
        (void)__sync_lock_test_and_set(&net_info->collector_addr.sin_family, tmp_addr.sin_family);
        inet_ntop(AF_INET, &net_info->collector_addr.sin_addr, ipstr, sizeof(ipstr));
        jrtc_logger(
            JRTC_INFO,
            "Updated output IP and UDP port for IO thread to %s:%d\n",
            ipstr,
            ntohs(net_info->collector_addr.sin_port));
    }

    return 0;
}

static int
_aa_net_open_output_socket(struct aa_net_info* net_info, char* fqdn, int out_port)
{

    if ((net_info->out_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        jrtc_logger(JRTC_ERROR, "Socket creation failed\n");
        return -1;
    } else {
        jrtc_logger(JRTC_INFO, "Output socket was created successfully\n");
    }

    /* Set port and IP: */
    net_info->collector_addr.sin_family = AF_INET;
    net_info->collector_addr.sin_port = htons(out_port);

    aa_net_update_fqdn(net_info);

    if (connect(net_info->out_sockfd, (struct sockaddr*)&net_info->collector_addr, sizeof(net_info->collector_addr)) <
        0) {
        jrtc_logger(JRTC_ERROR, "Could not connect to server\n");
        return -1;
    } else {
        jrtc_logger(JRTC_INFO, "The socket binding was successful\n");
    }
    return 0;
}

/* Opens a UDP socket to the analytics server */
static int
_aa_net_open_input_socket(struct aa_net_info* net_info, int in_port)
{
    socklen_t optlen;
    int sendbuff, r;

    if ((net_info->in_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        jrtc_logger(JRTC_ERROR, "Socket creation failed\n");
        return -1;
    }

    optlen = sizeof(sendbuff);
    r = getsockopt(net_info->in_sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);

    if (r == -1) {
        jrtc_logger(JRTC_ERROR, "Error getsockopt one\n");
    } else {
        jrtc_logger(JRTC_INFO, "Output UDP socket buffer size = %d Bytes\n", sendbuff);
    }

    memset(&net_info->control_input_addr, 0, sizeof(struct sockaddr_in));
    net_info->control_input_addr.sin_family = AF_INET;
    net_info->control_input_addr.sin_addr.s_addr = INADDR_ANY;
    net_info->control_input_addr.sin_port = htons(in_port);

    if (bind(net_info->in_sockfd, (const struct sockaddr*)&net_info->control_input_addr, sizeof(struct sockaddr)) < 0) {
        jrtc_logger(
            JRTC_ERROR,
            "Could not bind IO thread to UDP socket with port %d\n",
            ntohs(net_info->control_input_addr.sin_port));
        return -1;
    } else {
        jrtc_logger(
            JRTC_INFO,
            "Successful binding of IO thread to UDP socket with port %d\n",
            ntohs(net_info->control_input_addr.sin_port));
    }
    return 0;
}

int
aa_net_init(struct aa_net_info* net_info, int in_port, char* fqdn, int out_port)
{

    strncpy(net_info->fqdn, fqdn, AA_NET_MAX_FQDN_SIZE - 1);

    net_info->net_mempool = jbpf_init_mempool(AA_NET_NUM_DATA_ELEM, AA_NET_ELEM_SIZE, JBPF_MEMPOOL_SP);
    if (!net_info->net_mempool) {
        return -1;
    } else {
        jrtc_logger(JRTC_INFO, "IO memory mempool created successfully\n");
    }

    if (_aa_net_open_input_socket(net_info, in_port) != 0) {
        jbpf_destroy_mempool(net_info->net_mempool);
        return -1;
    }

    if (_aa_net_open_output_socket(net_info, fqdn, out_port) != 0) {
        jbpf_destroy_mempool(net_info->net_mempool);
        close(net_info->in_sockfd);
        return -1;
    }

    return 0;
}

void
aa_net_stop(struct aa_net_info* net_info)
{
    close(net_info->in_sockfd);
    close(net_info->out_sockfd);
}

void
aa_channel_encode_fwd_out_data(
    dapp_router_ctx_t dapp_ctx, jrtc_router_data_entry_t* data_entries, int num_entries, void* ctx)
{
    struct aa_net_info* net_info = ctx;

    uint8_t* data_to_write;
    jbpf_mbuf_t* jmbuf;
    jbpf_mbuf_t* jmbuf_list[AA_OUT_BUFS_BATCH_SIZE];
    int n_sent = 0;
    int msg_size, sent_messages;

    struct iovec iovec[AA_OUT_BUFS_BATCH_SIZE][1] = {0};
    struct mmsghdr datagrams[AA_OUT_BUFS_BATCH_SIZE] = {0};

    for (int i = 0; i < num_entries; i++) {

        msg_size = 0;

        jmbuf = jbpf_mbuf_alloc(net_info->net_mempool);

        if (!jmbuf) {
            jrtc_router_channel_release_buf(data_entries[i].data);
            jrtc_logger(JRTC_ERROR, "No memory to allocate for outbuf\n");
            continue;
        }

        data_to_write = jmbuf->data;

        msg_size = jrtc_router_create_serialized_msg(data_entries[i], data_to_write, AA_NET_ELEM_SIZE);

        if (msg_size) {
            iovec[n_sent][0].iov_base = data_to_write;
            iovec[n_sent][0].iov_len = msg_size;
            jmbuf_list[n_sent] = jmbuf;
            datagrams[n_sent].msg_hdr.msg_iov = iovec[n_sent];
            datagrams[n_sent].msg_hdr.msg_iovlen = 1;
            datagrams[n_sent].msg_hdr.msg_name = (struct sockaddr*)&net_info->collector_addr;
            datagrams[n_sent].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
            n_sent++;
        } else {
            jrtc_logger(JRTC_ERROR, "Encoding failed\n");
            jbpf_mbuf_free(jmbuf, false);
        }
        jrtc_router_channel_release_buf(data_entries[i].data);
    }

    sent_messages = sendmmsg(net_info->out_sockfd, datagrams, n_sent, 0);

    if (n_sent != sent_messages) {
        jrtc_logger(
            JRTC_ERROR, "Error sending all output messages. Had to send %d, but sent %d\n", n_sent, sent_messages);
        jrtc_logger(JRTC_ERROR, "Error: %s\n", strerror(errno));
    }

    for (int i = 0; i < n_sent; i++) {
        jbpf_mbuf_free(jmbuf_list[i], false);
    }

    return;
}

void*
jrtc_start_app(struct jrtc_app_env* env_ctx)
{
    jrtc_router_stream_id_t stream_id_req;
    int res, num_rcv;
    jrtc_router_data_entry_t data_entries[AA_OUT_BUFS_BATCH_SIZE] = {0};
    struct aa_net_info net_info = {0};

    jrtc_router_generate_stream_id(
        &stream_id_req,
        JRTC_ROUTER_DEST_UDP,
        JRTC_ROUTER_DEVICE_ID_ANY,
        JRTC_ROUTER_REQ_STREAM_PATH_ANY,
        JRTC_ROUTER_REQ_STREAM_NAME_ANY);

    jrtc_print_stream_id("The stream id is %s\n", &stream_id_req);
    jrtc_thread_set_scheduler(pthread_self(), &env_ctx->sched_config);

    char* out_fqdn = AA_NET_OUT_DEFAULT_FQDN;
    int in_port = AA_NET_IN_DEFAULT_PORT;
    int out_port = AA_NET_OUT_DEFAULT_PORT;

    char* env_aa_net_out_fqdn = getenv("AA_NET_OUT_FQDN");
    if (env_aa_net_out_fqdn != NULL) {
        out_fqdn = env_aa_net_out_fqdn;
    }
    const char* env_aa_net_in_port = getenv("AA_NET_IN_PORT");
    if (env_aa_net_in_port != NULL) {
        in_port = atoi(env_aa_net_in_port);
    }
    const char* env_aa_net_out_port = getenv("AA_NET_OUT_PORT");
    if (env_aa_net_out_port != NULL) {
        out_port = atoi(env_aa_net_out_port);
    }

    aa_net_init(&net_info, in_port, out_fqdn, out_port);

    res = jrtc_router_channel_register_stream_id_req(env_ctx->dapp_ctx, stream_id_req);

    if (res == 1) {
        jrtc_logger(JRTC_INFO, "Analytics Agent output app registered to router and listening...\n");
    } else {
        jrtc_logger(JRTC_ERROR, "Failed to register to router\n");
        exit(0);
    }

    jrtc_logger(JRTC_INFO, "Starting the jrtc_north_io app\n");
    while (!atomic_load(&env_ctx->app_exit)) {
        num_rcv = jrtc_router_receive(env_ctx->dapp_ctx, data_entries, AA_OUT_BUFS_BATCH_SIZE);
        if (num_rcv > 0) {
            aa_channel_encode_fwd_out_data(env_ctx->dapp_ctx, data_entries, num_rcv, &net_info);
        }

        usleep(100);
    }
    jrtc_logger(JRTC_INFO, "Exiting the jrtc_north_io app\n");

    aa_net_stop(&net_info);
    return NULL;
}