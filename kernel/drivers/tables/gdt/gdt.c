#include "gdt.h"
#include "../../../terminal/terminal.h"

struct gdt_descriptor_t
{
	uint16_t size;
	void *entries;
} __attribute((packed));

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0F

void vga_put_char(uint8_t x, uint8_t y, char c, uint8_t color) {
    volatile uint16_t* vga = (volatile uint16_t*)VGA_ADDRESS;
    vga[y * VGA_WIDTH + x] = (color << 8) | c;
}

// Print a buffer in hex to VGA starting at row 0, column 0
void print_buffer_hex_to_vga(uint32_t line, uint8_t* buffer, uint32_t size) {
    uint8_t x = 0;
    uint8_t y = line;

    for (uint32_t i = 0; i < size; i++) {
        uint8_t byte = buffer[i];
        char hex[2];

        // Convert byte to two hex characters
        hex[0] = "0123456789ABCDEF"[byte >> 4];
        hex[1] = "0123456789ABCDEF"[byte & 0x0F];

        // Print two hex chars
        vga_put_char(x++, y, hex[0], VGA_COLOR);
        if (x >= VGA_WIDTH) { x = 0; y++; }
        vga_put_char(x++, y, hex[1], VGA_COLOR);
        if (x >= VGA_WIDTH) { x = 0; y++; }

        // Add a space between bytes
        vga_put_char(x++, y, ' ', VGA_COLOR);
        if (x >= VGA_WIDTH) { x = 0; y++; }

        // Stop if we exceed VGA height
        if (y >= VGA_HEIGHT) break;
    }
}
void load_gdt( struct gdt_t gdt )
{
	struct gdt_descriptor_t descriptor;
	descriptor.size = 8*gdt.count - 1;
	descriptor.entries = gdt.entries;

	print_buffer_hex_to_vga(1, (uint8_t*)0x7e00, 8 * 3);
	print_buffer_hex_to_vga(5, (void*)gdt.entries, 8 * gdt.count);

	asm volatile(
			"lgdt %0\n"
			"ljmp $0x8, $1f\n"
			"1:\n"
			:: "m"(descriptor));
	printc("a\n", VGA_COLOR_BROWN);
}
