// ringbuffer.h

#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;    // buffer hot hold data [data size * dataSize]
    uint32_t head;      // Index of the first element
    uint32_t tail;      // Index one past the last element
    uint32_t size;      // Maximum number of elements in the buffer
    uint32_t count;     // Current number of elements in the buffer
    size_t dataSize;    // Size of each data element
    struct k_sem bmx;   // Mutex for mutual exclusion
} ring_buffer_t;

typedef void (*rb_scan_cb_t)(uint8_t *, uint32_t);
typedef void* (*rb_inject_cb_t)(void* accumulated_value, void* data);
typedef void (*rb_map_cb_t)(void * original, void * mapped);

// Function prototypes
ring_buffer_t * rb_init_ring_buffer(int size, size_t dataSize);
bool rb_enqueue(ring_buffer_t *cb, void *data);
bool rb_dequeue(ring_buffer_t *cb, void *data);
bool rb_dequeue_multiple(ring_buffer_t *cb, void *data, uint32_t numItems, uint32_t* dequeued);
void rb_scan(ring_buffer_t *cb, rb_scan_cb_t callback);
#define rb_each rb_scan
void* rb_inject(ring_buffer_t* cb, void* initial_value, rb_inject_cb_t callback);
#define rb_reduce rb_inject
ring_buffer_t* rb_map(ring_buffer_t* cb, rb_map_cb_t callback, size_t mappedDataSize);
bool rb_is_empty(ring_buffer_t *cb);
bool rb_is_full(ring_buffer_t *cb);
void rb_free_ring_buffer(ring_buffer_t *cb);

#ifdef __cplusplus
}
#endif

// end of ringbuffer.h
