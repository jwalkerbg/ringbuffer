// rungbuffer.c

#include "ringbuffer.h"

// History:
// v01.00, 12.03.2024
// Basic functionality ready.
// v01.10, 12.03.2024
// Added function rb_scan_buffer
// Changed types in ring_buffer_t to uint32_t
// v01.10, 13.03.2024
// Added function rb_inject

// Input:
//  size: maximal number of elements the ring can contain
//  dataSize: the size of one element.
// Output:
//  a pointer to a ringbuffer or NULL if there were no memory to allocate it.
// Description: This function initializes the ring buffer.
// dataSize is usually sizeof of some struct describing the data elements.
ring_buffer_t *rb_init_ring_buffer(int size, size_t dataSize)
{
    ring_buffer_t *cb = (ring_buffer_t *)malloc(sizeof(ring_buffer_t));
    if (cb == NULL) {
        return NULL; // Memory allocation failed
    }
    cb->buffer = malloc(size * dataSize);
    if (cb->buffer == NULL) {
        free(cb); // Clean up
        return NULL; // Memory allocation failed
    }
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    cb->dataSize = dataSize;
    k_sem_init(&cb->bmx,1,1);
    k_sem_give(&cb->bmx);
    return cb;
}

// Input:
// cb: pointer to a ringbuffer
// data: pointer to data (struct) that have to be inserted into the buffer
// Output:
//  true: the new element was enqueued; false: the new element was not enqueued because the buffer is full
// Description: This function adds an element to the ring buffer if there are space for it
bool rb_enqueue(ring_buffer_t *cb, void *data)
{
    int ret;

    k_sem_take(&cb->bmx,K_FOREVER);
    if (cb->count == cb->size) {
        ret = false;    // Buffer full
    }
    else {
        memcpy(cb->buffer + cb->tail, data, cb->dataSize);                  // Copy data to the ring buffer
        cb->tail = (cb->tail + cb->dataSize) % (cb->size * cb->dataSize);   // Update tail index with wrap-around
        cb->count++;
        ret = true;     // Success
    }
    k_sem_give(&cb->bmx);
    return ret;
}

// Input:
// cb: pointer to a ringbuffer
// data: pointer to data (struct) that will receive the dequeued data.
// Output:
//  true: the data was dequeued successfully; false: the buffer is empty, no data is dequeued.
// Description: This function removes an element from the ring buffer, that is the oldest one.
bool rb_dequeue(ring_buffer_t *cb, void *data)
{
    int ret;

    k_sem_take(&cb->bmx,K_FOREVER);
    if (cb->count == 0) {
        return false;   // Buffer empty
    }
    else {
        memcpy(data, cb->buffer + cb->head, cb->dataSize);                  // Copy data from the ring buffer
        cb->head = (cb->head + cb->dataSize) % (cb->size * cb->dataSize);   // Update head index with wrap-around
        cb->count--;
        ret = true;     // Success
    }
    k_sem_give(&cb->bmx);
    return ret;
}

// Input:
//  cb: pointer to a ringbuffer
//  data: pointer to data array that will receivce dequeued data
//  numItems: number of items to be dequeued
//  dequeued: a poiner to a variable that received the number of really dequeued elements
// Output:
//  true: elements were dequeued; false: no elements were dequeued
// Description: This function removes multiple elements from the ring buffer and copies them into data array.
bool rb_dequeue_multiple(ring_buffer_t *cb, void *data, uint32_t numItems, uint32_t* dequeued)
{
    k_sem_take(&cb->bmx,K_FOREVER); // Lock the mutex
    if (cb->count == 0) {
        if (dequeued) *dequeued = 0;
        k_sem_give(&cb->bmx);
        return false;   // Buffer empty, no items dequeued
    }

    // Calculate the number of items to dequeue
    uint32_t itemsToDequeue = (cb->count < numItems) ? cb->count : numItems;

    // Dequeue the items
    if (cb->head < cb->tail || cb->size == 1) {
        // No wraparound or single element buffer
        memcpy(data, cb->buffer + cb->head, itemsToDequeue * cb->dataSize);
    } else {
        // Wraparound
        uint32_t elementsToEnd = cb->size - cb->head / cb->dataSize;
        if (itemsToDequeue <= elementsToEnd) {
            // Dequeue within the same section
            memcpy(data, cb->buffer + cb->head, itemsToDequeue * cb->dataSize);
        } else {
            // Dequeue wraps around the buffer
            memcpy(data, cb->buffer + cb->head, elementsToEnd * cb->dataSize);
            memcpy((uint8_t*)data + (elementsToEnd * cb->dataSize), cb->buffer, (itemsToDequeue - elementsToEnd) * cb->dataSize);
        }
    }

    // Update head index with wrap-around
    cb->head = (cb->head + itemsToDequeue * cb->dataSize) % (cb->size * cb->dataSize);
    cb->count -= itemsToDequeue;

    if (dequeued) *dequeued = itemsToDequeue;
    k_sem_give(&cb->bmx);
    return true;
}

// Input:
//  cb: pointer to a ringbuffer
//  callback - pointer to a callback function to be called for each element in the buffer
// Output:
//  none
// Description: This function scans all elements in the buffer beginning from the oldest
// pointed to by cb->head till to the newest element, pointed to by cb->tail. It does not
// remove elements from the buffer. The callback function receives a pointer to uint8_t that
// points to the current selected element.
// The callback function is free to do what is suitable with its parameter, but it is forbideden
// to use the pointer to access other elements of the buffer.
void rb_scan(ring_buffer_t *cb, rb_scan_cb_t callback)
{
    k_sem_take(&cb->bmx,K_FOREVER); // Lock the mutex
    // Initialize index to head
    int currentIndex = cb->head / cb->dataSize;

    // Scan all available data in the buffer
    for (uint32_t i = 0; i < cb->count; ++i) {
        // Call the callback function with the current data element
        callback(cb->buffer + currentIndex * cb->dataSize,i);

        // Move to the next index, wrapping around if necessary
        currentIndex = (currentIndex + 1) % cb->size;
    }
    k_sem_give(&cb->bmx);
}

// Input:
//  cb: pointer to a ringbuffer
//  initial_value: pointer to a data struct that contains initial value
//  callback: pinter to a callback function that is called for each element in the buffer
// Output:
//  pinter to the initial data struct containing final value
// Description: This function mimics ruby's inject (reduce) method of enumerable objects.
// It performs reduction operation on the data in the ring buffer.
// The operation is defined by the callback function and the initial value (seed)
// The callback function takes two arguments: the current accumulated value and the current data element
// The function returns the final accumulated value, stored in the place of the initial value.
// The type of initial/final values may be different of the type of the values in the buffer.
void* rb_inject(ring_buffer_t* cb, void* initial_value, rb_inject_cb_t callback)
{
    k_sem_take(&cb->bmx,K_FOREVER); // Lock the mutex
    if (cb->count == 0) {
        k_sem_give(&cb->bmx);           // Unlock the mutex
        return initial_value;           // Return the initial value unchanged
    }

    // Initialize index to head
    int currentIndex = cb->head / cb->dataSize;

    // Initialize accumulated value to the initial value
    void *accumulatedValue = initial_value;

    // Perform the reduction operation on all elements in the buffer
    for (int i = 0; i < cb->count; ++i) {
        // Call the callback function with the current accumulated value and the current data element
        accumulatedValue = callback(accumulatedValue, cb->buffer + currentIndex * cb->dataSize);

        // Move to the next index, wrapping around if necessary
        currentIndex = (currentIndex + 1) % cb->size;
    }

    k_sem_give(&cb->bmx);
    return accumulatedValue;
}

// Input:
//  cb: pointer to a ringbuffer
//  callback: pointer to a callback function that is called for each element in the buffer
//  mappedDataSize: daa size of the new mapped data for the new ring buffer
// Output:
//  pointer to new ringbuffer, containing the new mapped data.
// Description: This function mimics ruby's map method of enumerable objects.
// It performs map operation - producing new data from the original data. From mathematical point of view
// if creates new function from the original function (mapping).
// The callback function takes two arguments: pointer to the curent original value and pointer to a space
// where the new value must be stored.
ring_buffer_t* rb_map(ring_buffer_t* cb, rb_map_cb_t callback, size_t mappedDataSize)
{
    k_sem_take(&cb->bmx,K_FOREVER); // Lock the mutex

    // Create a new circular buffer with the given size and data size
    ring_buffer_t* rb_mapped = rb_init_ring_buffer(cb->size,mappedDataSize);
    if (rb_mapped  == NULL) {
        // k_sem_give(&cb->bmx);
        return NULL;
    }

    // Allocate memory for the temporary new data element
    void *new_data_element = malloc(mappedDataSize);
    if (new_data_element == NULL) {
        rb_free_ring_buffer(rb_mapped); // Free the new circular buffer
        // k_sem_give(&cb->bmx); // Unlock the mutex
        return NULL; // Memory allocation failed
    }

    // Apply the callback function to each element of the original circular buffer and enqueue the results into the new circular buffer
    int currentIndex = cb->head / cb->dataSize;
    for (uint32_t i = 0; i < cb->count; ++i) {
        callback(cb->buffer + currentIndex * cb->dataSize, new_data_element);
        rb_enqueue(rb_mapped,new_data_element);
        // Move to the next index, wrapping around if necessary
        currentIndex = (currentIndex + 1) % cb->size;
    }

    // Free memory allocated for the temporary new data element
    free(new_data_element);

    // k_sem_give(&cb->bmx);
    return rb_mapped;
}

// Input:
//  cb: pointer to a ringbuffer
// Output:
//  true: the buffer is empty; false: buffer is not empty
// Description: This function checks if the ring buffer is empty.
bool rb_is_empty(ring_buffer_t *cb)
{
    return (cb->count == 0);
}

// Input:
//  cb: pointer to a ringbuffer
// Output:
//  true: the buffer is full; false: the buffer is not full
// Description: This function checks if the ring buffer is full.
bool rb_is_full(ring_buffer_t *cb)
{
    return (cb->count == cb->size);
}

// Input:
//  cb: pointer to a ringbuffer
// Output:
//  none
// Description: This function frees the memory allocated for the ring buffer.
// Any data contained in the buffer are discarded. The pointer to the ring buffer
// becomes invalid and should not be used.
void rb_free_ring_buffer(ring_buffer_t *cb)
{
    if (cb != NULL) {
        if (cb->buffer != NULL) {
            free(cb->buffer);
        }
        free(cb);
    }
}

// end of ringbuffer.c
