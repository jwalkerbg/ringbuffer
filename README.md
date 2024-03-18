# Ring buffer

## Introduction

Ringbuffer is a container of data with FIFO organization. It is designed for data of single type, stored in an internal buffer as array. Ring buffer is thread-safe. Operations on data are protected by a semaphore. Eaxch instansce of ring bufer has its own sepmaphore. Before using ring buffer it is initialized - its control structure is allocated and linked to it an internal buffer is allocated for data array. At the end, there is a function that deallocates the memory used by the buffer. Data buffer cannot change its size, it is not provided, so the user must decice how much data will keep inside the buffer.

The data is represented as a C struct, and the volume of the ring buffer is calculated as the number of elements of that struct contained in it.

Ring buffer has usual access functions to enqueue (push) new elements and dequeue (pop) oldest elements. As extra, there are functions to inspect and manipulate data in the buffer without getting out it. These functions use callback functions, supplied by the user and this enables to create various actions with the data. Some of them follow ideas from ruby language functions with blocks. Below are presented examples using callbacks.

## API

Functions have annotations above its implementation. See them to figure out what thee functions do. Here is given brief description of each of them. All functions except have the buffer handle as a first argument: ```ring_buffer_t *cb```.

### ring_buffer_t * rb_init_ring_buffer(int size, size_t dataSize);

Description: This function initializes a ring buffer, returning a pointer to it (handle) if successful, otherwise it returns ```NULL```. The argument ```size``` determines how many elements the buffer can keep inside, and ```dataSize``` determines how big is each element. Usually this is ```sizeof()``` of the C struct representing the data.

### bool rb_enqueue(ring_buffer_t *cb, void *data);

Description: This function enuqueues (pushes) and element into the buffer and returns ```true``` if successful in this. Otherwise, if there is no room for new data (the buffer is full) it returns ```false```. The data is _copied_ from the space pointed to by ```data``` into the buffer internal buffer.

### bool rb_dequeue(ring_buffer_t *cb, void *data);

Description: This function dequeues oldest element from the buffer and _copies_ it into a space pointed to by ```data``` parameter.

### bool rb_dequeue_multiple(ring_buffer_t* cb, void* data, int numItems, int* dequeued);

Description: This function dequeues multiple elements from the buffer _copying_ them into ```data```. It tries to copy ```numItems```. If there are less than ```numItems``` inside the buffer, it _copies_ as many as possible. The function updates ```*dequeued``` with the number of actually dequeued elements.

### void rb_scan_buffer(ring_buffer_t *cb, rb_scan_cb_t callback);

Description: This function scans all elements present in the buffer beginning from the oldest one and finishing with the newest one. For each one the function calls supplied as a parameter callback function. This callback receives a pointer to the current element (reference) and index of the element. The first call will supply the oldest element with index ```0``` and the last call will supply the newest element with ```ring_buffer_t::count - 1```.

The callback function can be used for what is needed by the application, even to change data in elements. Par example if elements are structures with three integers ```a```, ```b``` and ```c```, the callback function can store in ```c``` the value of ```a + b```.

Obviously, ```rb_scan_buffer``` can be used and for trivial tasks as to print elements data to output or save them to a file.

For tasks where a single calculated result is needed, without changing the elements (sum, product, average etc...) ```rb_scan_buffer``` can do them using external variables, however ```rb_inject``` is more suitable for such tasks.

### void* rb_inject(ring_buffer_t* cb, void* initial_value, rb_inject_t callback);

Description: This function

### bool rb_is_empty(ring_buffer_t *cb);

Description: This function returns ```true``` if the buffer is empty otherwise it returns ```false```.

### bool rb_is_full(ring_buffer_t *cb);

Description: This function returns ```true``` if the buffer is full otherwise it returns ```false```.

### void rb_free_ring_buffer(ring_buffer_t *cb);

Description: This function frees the memory occupied by the buffer. Any data contained in it is irreversibly lost.

## Examples

### Scan and log data in the buffer

### Find sum of the data in the buffer

### Calculate maginute of a vector in 3D space from its x, y, z coordinates

### Find average of the data in the buffer