#include "kernel.h"
#include "drivers/vga.h"

void _entry()
{
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    puts("--- Community OS v0.1 ---\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    puts("Built by random people on the internet.\n\n");

    /* Hangs forever */
    for (;;) {}
}
