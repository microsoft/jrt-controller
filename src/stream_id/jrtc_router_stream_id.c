// Copyright (c) Microsoft Corporation. All rights reserved.
#include "jrtc_router_stream_id.h"

#include "jrtc_logging.h"
#include "stream_id_hash.h"

typedef struct jrtc_router_bf
{
    jrtc_router_bitmap_t* bitmap;
    int bit_size;
    int n_hash;
} jrtc_router_bf_t;

static inline void
jrtc_router_stream_id_print_bits(size_t const size, void const* const ptr)
{
    unsigned char* b = (unsigned char*)ptr;
    unsigned char byte;
    int i, j;

    for (i = 0; i < size; i++) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

static inline bool
jrtc_router_bf_init(jrtc_router_bf_t* bf, int bit_size, int n_hash)
{

    bf->bitmap = jrtc_router_bitmap_create(bit_size);

    if (!bf->bitmap) {
        return false;
    }

    bf->bit_size = bit_size;
    bf->n_hash = n_hash;
    return true;
}

static inline void
jrtc_router_bf_destroy(jrtc_router_bf_t* bf)
{
    if (!bf) {
        return;
    }
    jrtc_router_bitmap_destroy(bf->bitmap);
}

static inline void
jrtc_router_bf_add(jrtc_router_bf_t* bf, const void* elem, size_t elem_size)
{
    if (!bf) {
        jrtc_logger(JRTC_ERROR, "No filter given!!!\n");
        return;
    }

    for (int i = 0; i < bf->n_hash; i++) {
        uint64_t hash = MurmurHash64A(elem, elem_size, i);
        hash %= bf->bit_size;
        jrtc_router_bitmap_set(bf->bitmap, hash);
    }
}

static inline bool
jrtc_router_bf_contains(jrtc_router_bf_t* bf, const void* elem, size_t elem_size)
{
    if (!bf) {
        jrtc_logger(JRTC_ERROR, "No filter given!!!\n");
        return false;
    }

    for (int i = 0; i < bf->n_hash; i++) {
        uint64_t hash = MurmurHash64A(elem, elem_size, i);
        hash %= bf->bit_size;
        if (!jrtc_router_bitmap_get(bf->bitmap, hash)) {
            return false;
        }
    }

    return true;
}

// Function to extract the first N bits from an array of bytes
static uint64_t
jrtc_router_bf_extract_bits(jrtc_router_bf_t* bf, int n_bits)
{

    uint64_t result = 0;
    unsigned int i = 0, bits = 0;
    jrtc_router_bitmap_iterator_t iter;

    if (n_bits > 64 || bf->bit_size < n_bits) {
        return 0;
    }

    jrtc_router_bitmap_init_iterator(&iter, bf->bitmap);

    while (jrtc_router_bitmap_iterator_has_next(&iter)) {
        bits++;
        if (bits > n_bits) {
            break;
        }
        i = jrtc_router_bitmap_iterator_next(&iter);
        result |= 1L << i;
    }

    return result;
}

static uint64_t
jrtc_router_get_hash(const char* name)
{
    jrtc_router_bf_t bf;
    int name_len;
    uint64_t value;

    if (!jrtc_router_bf_init(&bf, JRTC_ROUTER_HASH_NUMBER_BITS, JRTC_ROUTER_NUM_HASH_FUNCTIONS)) {
        return 0;
    }

    name_len = strlen(name);
    jrtc_router_bf_add(&bf, name, name_len);
    value = jrtc_router_bf_extract_bits(&bf, JRTC_ROUTER_HASH_NUMBER_BITS);

    jrtc_router_bf_destroy(&bf);

    return value;
}

void
jrtc_router_stream_id_set_ver(jrtc_router_stream_id_t* stream_id, uint16_t ver)
{
    if (!stream_id) {
        return;
    }

    _jrtc_router_stream_id_set_ver(stream_id->id, ver);
}

void
jrtc_router_stream_id_set_fwd_dst(jrtc_router_stream_id_t* stream_id, uint16_t fwd_dst)
{
    if (!stream_id) {
        return;
    }

    _jrtc_router_stream_id_set_fwd_dst(stream_id->id, fwd_dst);
}

void
jrtc_router_stream_id_set_device_id(jrtc_router_stream_id_t* stream_id, uint16_t device_id)
{
    if (!stream_id) {
        return;
    }

    _jrtc_router_stream_id_set_device_id(stream_id->id, device_id);
}

void
jrtc_router_stream_id_set_stream_path(jrtc_router_stream_id_t* stream_id, uint64_t stream_path)
{
    if (!stream_id) {
        return;
    }

    _jrtc_router_stream_id_set_stream_path(stream_id->id, stream_path);
}

void
jrtc_router_stream_id_set_stream_name(jrtc_router_stream_id_t* stream_id, uint64_t stream_name)
{
    if (!stream_id) {
        return;
    }

    _jrtc_router_stream_id_set_stream_name(stream_id->id, stream_name);
}

uint16_t
jrtc_router_stream_id_get_ver(const jrtc_router_stream_id_t* stream_id)
{
    if (!stream_id) {
        return 0;
    }

    return _jrtc_router_stream_id_get_ver(stream_id->id);
}

uint16_t
jrtc_router_stream_id_get_fwd_dst(const jrtc_router_stream_id_t* stream_id)
{
    if (!stream_id) {
        return 0;
    }

    return _jrtc_router_stream_id_get_fwd_dst(stream_id->id);
}

uint16_t
jrtc_router_stream_id_get_device_id(const jrtc_router_stream_id_t* stream_id)
{
    if (!stream_id) {
        return 0;
    }

    return _jrtc_router_stream_id_get_device_id(stream_id->id);
}

uint64_t
jrtc_router_stream_id_get_stream_path(const jrtc_router_stream_id_t* stream_id)
{
    if (!stream_id) {
        return 0;
    }

    return _jrtc_router_stream_id_get_stream_path(stream_id->id);
}

uint64_t
jrtc_router_stream_id_get_stream_name(const jrtc_router_stream_id_t* stream_id)
{
    if (!stream_id) {
        return 0;
    }

    return _jrtc_router_stream_id_get_stream_name(stream_id->id);
}

int
jrtc_router_generate_stream_id(
    jrtc_router_stream_id_t* stream_id, int fwd_dst, int device_id, const char* stream_path, const char* stream_name)
{

    uint64_t path_hash, name_hash;
    struct jrtc_router_stream_id_int sid;

    if (!stream_id) {
        return -1;
    }

    memset(stream_id, 0, sizeof(jrtc_router_stream_id_t));

    sid.ver = JRTC_ROUTER_STREAM_ID_VERSION;
    sid.fwd_dst = fwd_dst;
    sid.device_id = device_id;

    if (!stream_path) {
        path_hash = JRTC_ROUTER_STREAM_PATH_ANY;
    } else {
        path_hash = jrtc_router_get_hash(stream_path);
    }

    if (!stream_name) {
        name_hash = JRTC_ROUTER_STREAM_NAME_ANY;
    } else {
        name_hash = jrtc_router_get_hash(stream_name);
    }

    if (!path_hash || !name_hash) {
        return -1;
    }

    sid.stream_name = name_hash;
    sid.stream_path = path_hash;

    _jrtc_router_stream_id_set_ver(stream_id->id, sid.ver);
    _jrtc_router_stream_id_set_fwd_dst(stream_id->id, sid.fwd_dst);
    _jrtc_router_stream_id_set_device_id(stream_id->id, sid.device_id);
    _jrtc_router_stream_id_set_stream_path(stream_id->id, sid.stream_path);
    _jrtc_router_stream_id_set_stream_name(stream_id->id, sid.stream_name);

    return 1;
}
