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

### void rb_scan(ring_buffer_t *cb, rb_scan_cb_t callback);

Description: This function scans all elements present in the buffer beginning from the oldest one and finishing with the newest one. For each one the function calls supplied as a parameter callback function. The callback receives a pointer to the current element (reference) and index of the element. The first call will supply the oldest element with index ```0``` and the last call will supply the newest element with ```ring_buffer_t::count - 1```.

The callback function can be used for what is needed by the application, even to change data in elements. Par example if elements are structures with three integers ```a```, ```b``` and ```c```, the callback function can store in ```c``` the value of ```a + b```.

Obviously, ```rb_scan``` can be used and for trivial tasks as to print elements data to output or save them to a file.

For tasks where a single calculated result is needed, without changing the elements (sum, product, average etc...) ```rb_scan``` can do them using external variables, however ```rb_inject``` is more suitable for such tasks.

```rb_each``` is a synonym of ```rb_scan```.

### void* rb_inject(ring_buffer_t* cb, void* initial_value, rb_inject_cb_t callback);

Description: This function produces single result from the data in the buffer performing operation(s) on them. Simple example is to calculate and return the sum of the elements (if they are summable). The function scans all elements beginning from the oldest one and finishing with the newest one. The callback function takes two arguments: the current accumulated value and the current data element. It performs operations on the current value and changes the acummulated value with the produced result. The function returns the final accumulated value, stored in the place of the initial value. The type of initial/final values can be different from the type of the values in the buffer.

The idea for ```rb_inject``` comes from the function with same name in Ruby language. ```rb_reduce``` is a synonym of ```rb_inject```.

### ring_buffer_t* rb_map(ring_buffer_t* cb, rb_map_cb_t callback, size_t mappedDataSize);

Description: This function mimics ruby's map method of enumerable objects. It performs map operation - producing new data from the original data. From mathematical point of view if creates new function from the original function (mapping). The callback function takes two arguments: pointer to the curent original value and pointer to a space where the new value must be stored.

### ring_buffer_t* rb_select(ring_buffer_t* cb, rb_select_cb_t callback);

Description: This function mimics ruby's select method of enumerable object. ```rb_select``` calls the callback function with each element. The callback does not change the element. Instead, it inspects it and returns ```false``` (element not selected) or ```true``` (element selected). All selected elements are copied (not moved) to a new ring buffer with the same size. After scanning all elements ```rb_select``` returns a pointer to the new ring buffer. If no elements are selected, the new buffer does not contain elements. If there is no memory for a new buffer the function returns NULL.

### bool rb_all(ring_buffer_t* cb, rb_select_cb_t callback);

Description: This function calls the callback function with each element. If the callback returns ```true``` for all elements, ```rb_all``` returns ```true``` else it returns ```false```.

### bool rb_any(ring_buffer_t* cb, rb_select_cb_t callback);

Description: This function calls the callback function with each element. If the callback returns ```true``` for at least for one element, ```rb_any``` returns ```true```. If the callback returns ```false``` for all elements, ```rb_any``` returns ```false```.

### bool rb_one(ring_buffer_t* cb, rb_select_cb_t callback);

Description: This function calls the callback function with each element. If the callback returns ```true``` for exactly one element, ```rb_one``` returns ```true```. Otherwise ```rb_one``` returns ```false```.

### bool rb_is_empty(ring_buffer_t *cb);

Description: This function returns ```true``` if the buffer is empty otherwise it returns ```false```.

### bool rb_is_full(ring_buffer_t *cb);

Description: This function returns ```true``` if the buffer is full otherwise it returns ```false```.

### void rb_free_ring_buffer(ring_buffer_t *cb);

Description: This function frees the memory occupied by the buffer. Any data contained in it is irreversibly lost.

## Examples

### Scan and log data in the buffer

```
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    double magnitude;
} point3d_t;

void log_callback(uint8_t * item, uint32_t index)
{
    point3d_t* p = (point3d_t* )item;
    std::cout << "Scanned point3d_t data[" << index << "]: x=" << p->x << " y=" << p->y << " z=" << p->z << std::endl;
}

int main(void)
{
    // Enqueue some point3d_t elements
    point3d_t point_data1 = {10, 10, 10};
    point3d_t point_data2 = {20, 20, 20};
    point3d_t point_data3 = {30, 30, 30};
    point3d_t point_data3 = {40, 40, 40};

    ring_buffer_t *rb_point3d = rb_init_ring_buffer(4, sizeof(point3d_t));
    if (rb_point3d == NULL) {
        std::cout << "Failed to initialize ring buffer for point3d_t" << std::endl;
        return 1;
    }

    rb_enqueue(rb_point3d, &point_data1);
    rb_enqueue(rb_point3d, &point_data2);
    rb_enqueue(rb_point3d, &point_data3);
    rb_enqueue(rb_point3d, &point_data4);

    rb_scan(rb_point3d,log_callback);

    rb_free_ring_buffer(rb_point3d);

    return 0;
}
```

### Find sum of the data in the buffer

```
typedef struct {
    int16_t value;
} value_t;

void* sum_callback(void *accumulatedValue, void *data)
{
    int32_t* p = (int32_t* )accumulatedValue;
    value_t* element = (value_t *)data;
    *p += element->value;
}

int main(void)
{
    // Enqueue some point3d_t elements
    value_t point_data1 = 10;
    value_t point_data2 = 20;
    value_t point_data3 = 30;
    value_t point_data3 = 40;

    ring_buffer_t *rb_value = rb_init_ring_buffer(4, sizeof(rb_value));
    if (rb_point3d == NULL) {
        std::cout << "Failed to initialize ring buffer for point3d_t" << std::endl;
        return 1;
    }

    rb_enqueue(rb_value, &point_data1);
    rb_enqueue(rb_value, &point_data2);
    rb_enqueue(rb_value, &point_data3);
    rb_enqueue(rb_value, &point_data4);

    int32_t sum = 0;   // initial value
    rb_inject(rb_value,&sum,sum_callback);
    std::cout << Sum = " << sum << std::endl;

    rb_free_ring_buffer(rb_point3d);

    return 0;
}
```

### Calculate magninute of a vector in 3D space from its x, y, z coordinates

```
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    double magnitude;
} point3d_t;

void mag_callback(uint8_t * item, uint32_t index)
{
    point3d_t* p = (point3d_t* )item;
    p->magnitude = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
    std::cout << "Magnitude = " << p->magnitude << std::endl;
}

int main(void)
{
    // Enqueue some point3d_t elements
    point3d_t point_data1 = {10, 10, 10};
    point3d_t point_data2 = {20, 20, 20};
    point3d_t point_data3 = {30, 30, 30};
    point3d_t point_data3 = {40, 40, 40};

    ring_buffer_t *rb_point3d = rb_init_ring_buffer(4, sizeof(point3d_t));
    if (rb_point3d == NULL) {
        std::cout << "Failed to initialize ring buffer for point3d_t" << std::endl;
        return 1;
    }

    rb_enqueue(rb_point3d, &point_data1);
    rb_enqueue(rb_point3d, &point_data2);
    rb_enqueue(rb_point3d, &point_data3);
    rb_enqueue(rb_point3d, &point_data4);

    rb_scan(rb_point3d,mag_callback);

    rb_free_ring_buffer(rb_point3d);

    return 0;
}
```

### Find average of the data in the buffer

```
typedef struct {
    double value;
} mag_t;

typedef struct {
    double sum;
    double avg;
    uint32_t count;
} rb_avg_t;

void avg_callback(void *accumulatedValue, void *data)
{
    rb_avg_t* p = (rb_avg_t* )accumulatedValue;
    mag_t* element = (mag_t *)data;
    p->sum += element->value;
    p->count += 1;
}

int main(void)
{
    // Enqueue some point3d_t elements
    mag_t point_data1.value = 10.0;
    mag_t point_data2.value = 20.0;
    mag_t point_data3.value = 30.0;
    mag_t point_data3.value = 40.0;

    ring_buffer_t *rb_mag = rb_init_ring_buffer(4, sizeof(mag_t));
    if (rb_mag == NULL) {
        std::cout << "Failed to initialize ring buffer for mag_t" << std::endl;
        return 1;
    }

    rb_enqueue(rb_mag, &point_data1);
    rb_enqueue(rb_mag, &point_data2);
    rb_enqueue(rb_mag, &point_data3);
    rb_enqueue(rb_mag, &point_data4);

    rb_avg_t avg;
    avg.sum = 0.0;
    avg.count = 0;
    rb_inject(rb_mag,&avg,avg_callback);
    avg.avg = avg.sum / avg.count;
    std::count << "Average = " << avg.avg << std::endl;

    rb_free_ring_buffer(rb_point3d);

    return 0;
}
```

### Turn a vector to opposite direction.

```
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    double magnitude;
} point3d_t;

void mag_callback(uint8_t * item, uint32_t index)
{
    point3d_t* p = (point3d_t* )item;
    p->magnitude = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
    std::cout << "Magnitude = " << p->magnitude << std::endl;
}

void rb_mapping_callback(void * original, void * mapped)
{
    ((point3d_t* )mapped)->x = -((point3d_t* )original)->x;
    ((point3d_t* )mapped)->y = -((point3d_t* )original)->y;
    ((point3d_t* )mapped)->z = -((point3d_t* )original)->z;
    ((point3d_t* )mapped)->magnitude = ((point3d_t* )original)->magnitude;
}

void print_callback(uint8_t * item, uint32_t index)
{
    point3d_t* p = (point3d_t* )item;

    std::cout << "x = " << p->x << " y = " << p->y << " z = " << p->z << " Magnitude = " << p->magnitude << std::endl;
}

int main(void)
{
    // Enqueue some point3d_t elements
    point3d_t point_data1 = {10, 10, 10, 0};
    point3d_t point_data2 = {20, 20, 20, 0};
    point3d_t point_data3 = {30, 30, 30, 0};
    point3d_t point_data3 = {40, 40, 40, 0};

    ring_buffer_t *rb_point3d = rb_init_ring_buffer(4, sizeof(point3d_t));
    if (rb_point3d == NULL) {
        std::cout << "Failed to initialize ring buffer for point3d_t" << std::endl;
        return 1;
    }

    rb_enqueue(rb_point3d, &point_data1);
    rb_enqueue(rb_point3d, &point_data2);
    rb_enqueue(rb_point3d, &point_data3);
    rb_enqueue(rb_point3d, &point_data4);

    rb_scan(rb_point3d,mag_callback);

    ring_buffer_t* rb_opposite = rb_map(rb_point3d,rb_mapping_callback,sizeof(point3d_t));
    if (rb_opposite != NULL) {
        rb_scan(rb_point3d,print_callback);
        rb_free_ring_buffer(rb_opposite);
    }

    rb_free_ring_buffer(rb_point3d);

    return 0;
}
```

### Select even numbers from a ringbuffer.

```
bool rb_selected_callback(void * data)
{
    if ((*((int* )data) % 2) == 0) {
        return true;
    }
    return false;
}

void rb_scan_int_callback(uint8_t* element, uint32_t index)
{
    int* ptr = (int*)element;
    std::cout << index << ": value = " << *ptr << std::endl;
}

#definbe RBSIZE (10)
int main(void)
{
    ring_buffer_t* rb_handle = NULL;

    // create buffer with RBSIZE elements
    if ((rb_handle = rb_init_ring_buffer(RBSIZE,sizeof(int))) == NULL) {
        std::cout << "No memory to allocate ring buffer." << std::endl;
        return 1;
    }

    // fill the buffer
    for (int i = 0; i < RBSIZE; i++) {
        if (rb_is_full(rb_handle)) {
            int deq;
            rb_dequeue(rb_handle,&deq);
        }
        rb_enqueue(rb_handle,&i);
    }

    // select even elements in new buffer, pointed to by rb_selected, then print it to std::cout
    ring_buffer_t* rb_selected = rb_select(rb_handle,rb_selected_callback);
    rb_scan_buffer(rb_selected,rb_scan_int_callback);

    rb_free_ring_buffer((rb_handle));

    return 0;
}
```