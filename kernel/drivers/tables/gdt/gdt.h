#ifndef GDT_H
#define GDT_H
#include "stdint.h"

struct gdt_entry_t
{

    unsigned short limit_low;
    unsigned int base_low:24;

    unsigned char access : 1;
    unsigned char read_write : 1;
    unsigned char direction_conform : 1;
    unsigned char code : 1;

    unsigned char not_system : 1;
    unsigned char ring : 2;
    unsigned char present : 1;

    unsigned char limit_high:4;

    unsigned char reserved:1;
    unsigned char long_64bit:1;
    unsigned char big_32bit:1;
    unsigned char gran_4kb:1;

    unsigned char base_high;
} __attribute__((packed));

struct gdt_t
{
	struct gdt_entry_t *entries;
	uint16_t count;
};

static inline void gdt_entry_write_base( struct gdt_entry_t *entry, uint32_t base )
{
	entry->base_low = base;
	entry->base_high = (base & (0xff << 24)) >> 24;
}

static inline void gdt_entry_write_limit( struct gdt_entry_t *entry, uint32_t limit )
{
	entry->limit_low = limit;
	entry->limit_high = (limit & (0xf << 16)) >> 16;

}

void load_gdt( struct gdt_t gdt );

#endif
