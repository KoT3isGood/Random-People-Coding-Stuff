// bonk enjoyer (dorito girl)

// Should i write down everything i did? (Pumpkicks)
#ifndef _MEM_H
#define _MEM_H



void* memcpy(void* dest, const void* src, unsigned long n);

//Ember2819
void* memset(void* dest, int val, unsigned long n);

void kalloc_init();
void* kmalloc(unsigned long size);

//Pumpkicks
extern unsigned long memend; // The memory size from the 0x1000
int strlen(char* ptr);

typedef unsigned int size_t;
typedef int ssize_t;

#endif
