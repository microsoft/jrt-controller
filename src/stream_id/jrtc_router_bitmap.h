// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef JRTC_ROUTER_BITMAP_H
#define JRTC_ROUTER_BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * @brief Define a structure to represent the bit array
 * @ingroup stream_id
 * size: Size of the bit array in bits
 * data: Array to store the bits
 */
typedef struct
{
    uint32_t size;  // Size of the bit array in bits
    uint32_t* data; // Array to store the bits
} jrtc_router_bitmap_t;

/**
 * @brief Define a structure to represent the bit array iterator
 * @ingroup stream_id
 * bitmap: Reference to the bit array being iterated
 * index: Current index in the iteration
 */
typedef struct
{
    const jrtc_router_bitmap_t* bitmap; // Reference to the bit array being iterated
    uint32_t index;                     // Current index in the iteration
} jrtc_router_bitmap_iterator_t;

/**
 * @brief Create a new bit array
 * @ingroup stream_id
 * @param size The size of the bit array
 * @return A pointer to the newly created bit array
 */
static inline jrtc_router_bitmap_t*
jrtc_router_bitmap_create(uint32_t size)
{
    jrtc_router_bitmap_t* bitmap = (jrtc_router_bitmap_t*)malloc(sizeof(jrtc_router_bitmap_t));
    if (!bitmap) {
        return NULL;
    }

    bitmap->size = size;
    // Allocate memory for the bit data (assuming each element is 32 bits)
    bitmap->data = (uint32_t*)malloc((size + 31) / 32 * sizeof(uint32_t));
    if (bitmap->data == NULL) {
        free(bitmap);
        return NULL;
    }

    // Initialize all bits to 0
    for (uint32_t i = 0; i < (size + 31) / 32; ++i) {
        bitmap->data[i] = 0;
    }

    return bitmap;
}

/**
 * @brief Set a specific bit in the bit array
 * @ingroup stream_id
 * @param bitmap The bit array
 * @param index The index of the bit to set
 */
static inline void
jrtc_router_bitmap_set(jrtc_router_bitmap_t* bitmap, uint32_t index)
{
    if (!bitmap) {
        return;
    }

    if (index < bitmap->size) {
        bitmap->data[index / 32] |= (1U << (index % 32));
    }
}

/**
 * @brief Get the value of a specific bit in the bit array
 * @ingroup stream_id
 * @param bitmap The bit array
 * @param index The index of the bit to get
 * @return The value of the bit
 */
static inline int
jrtc_router_bitmap_get(const jrtc_router_bitmap_t* bitmap, uint32_t index)
{
    if (!bitmap) {
        return -1;
    }

    if (index < bitmap->size) {
        return (bitmap->data[index / 32] & (1U << (index % 32))) != 0;
    }

    return -1;
}

/**
 * @brief Initialize the bit array iterator
 * @ingroup stream_id
 * @param iterator The iterator
 * @param bitmap The bit array
 */
static inline void
jrtc_router_bitmap_init_iterator(jrtc_router_bitmap_iterator_t* iterator, const jrtc_router_bitmap_t* bitmap)
{
    iterator->bitmap = bitmap;
    iterator->index = 0;
}

/**
 * @brief Check if there are more bits to iterate
 * @ingroup stream_id
 * @param iterator The iterator
 * @return 1 if there are more bits to iterate, 0 otherwise
 */
static inline int
jrtc_router_bitmap_iterator_has_next(const jrtc_router_bitmap_iterator_t* iterator)
{

    jrtc_router_bitmap_iterator_t iter = *iterator;

    while (iter.index < iter.bitmap->size) {
        if (jrtc_router_bitmap_get(iter.bitmap, iter.index)) {
            return 1;
        }
        iter.index++;
    }
    return 0;
}

/**
 * @brief Get the next bit in the iteration
 * @ingroup stream_id
 * @param iterator The iterator
 * @return The index of the next bit
 */
static inline uint32_t
jrtc_router_bitmap_iterator_next(jrtc_router_bitmap_iterator_t* iterator)
{

    while (iterator->index < iterator->bitmap->size) {
        if (jrtc_router_bitmap_get(iterator->bitmap, iterator->index)) {
            return iterator->index++;
        }
        iterator->index++;
    }
    return -1;
}

/**
 * @brief Destroy the bit array
 * @ingroup stream_id
 * @param bitmap The bit array
 */
static inline void
jrtc_router_bitmap_destroy(jrtc_router_bitmap_t* bitmap)
{
    free(bitmap->data);
    free(bitmap);
}

#endif
