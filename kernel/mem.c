void* memcpy(void* dest, const void* src, unsigned long n) {
    unsigned char* d = dest;
    const unsigned char* s = src;

    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}
