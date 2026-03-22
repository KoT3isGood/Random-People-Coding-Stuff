void* memcpy(void* dest, const void* src, unsigned long n) {
    // n = Number of bytes

    unsigned char* d = dest;
    const unsigned char* s = src;

    // Iterate n times, copying the byte in s into the same index in d
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

// [Ember2819: BEGIN - memset implementation]
void* memset(void* dest, int val, unsigned long n) {
    unsigned char* d = dest;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = (unsigned char)val;
    }
    return dest;
}
// [Ember2819: END]
//replace with real allocator later but should be fine for now
static unsigned char heap[65536];
static unsigned long heap_ptr = 0;

void* malloc(unsigned long size) {
    if (heap_ptr + size > sizeof(heap)) {
        return 0;
    }
    void* p = &heap[heap_ptr];
    heap_ptr += size;
    return p;
}
