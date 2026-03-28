//#include "kernel.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/idt/idt.h"
#include "drivers/tables/irq/irq.h"
#include "drivers/tables/timer/timer.h"
#include "drivers/tables/gdt/gdt.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/drives.h"
#include "layouts/kb_layouts.h"
#include "terminal/terminal.h"
#include "commands.h" // Included by Ember2819: Adds commands
#include "colors.h" // Added by MorganPG1 to centralise colors into one file
#include <stdint.h>

// Ember2819: Add command functionality
void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

static void kmain();

static struct gdt_t gdt;
static struct gdt_entry_t gdt_entries[6];
struct tss_entry_t {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __packed;

struct tss_entry_t tss_entry;

__attribute__((section(".text.entry"))) // Add section attribute so linker knows this should be at the start
void _entry() {


	gdt.count = 6;
	gdt.entries = gdt_entries;

	memset(gdt_entries, 0, sizeof(gdt_entries));

	gdt_entry_write_base(&gdt_entries[1], 0);
	gdt_entry_write_limit(&gdt_entries[1], 0xFFFFF);
	gdt_entries[1].read_write = 1;
	gdt_entries[1].code = 1;
	gdt_entries[1].not_system = 1;
	gdt_entries[1].present = 1;
	gdt_entries[1].gran_4kb = 1;
	gdt_entries[1].big_32bit = 1;

	gdt_entry_write_base(&gdt_entries[2], 0);
	gdt_entry_write_limit(&gdt_entries[2], 0xFFFFF);
	gdt_entries[2].read_write = 1;
	gdt_entries[2].present = 1;
	gdt_entries[2].not_system = 1;
	gdt_entries[2].gran_4kb = 1;
	gdt_entries[2].big_32bit = 1;

	gdt_entry_write_base(&gdt_entries[3], 0);
	gdt_entry_write_limit(&gdt_entries[3], 0xFFFFF);
	gdt_entries[3].read_write = 1;
	gdt_entries[3].code = 1;
	gdt_entries[3].ring = 3;
	gdt_entries[3].not_system = 1;
	gdt_entries[3].present = 1;
	gdt_entries[3].gran_4kb = 1;
	gdt_entries[3].big_32bit = 1;

	gdt_entry_write_base(&gdt_entries[4], 0);
	gdt_entry_write_limit(&gdt_entries[4], 0xFFFFF);
	gdt_entries[4].read_write = 1;
	gdt_entries[4].present = 1;
	gdt_entries[4].ring = 3;
	gdt_entries[4].not_system = 1;
	gdt_entries[4].gran_4kb = 1;
	gdt_entries[4].big_32bit = 1;

	memset(&tss_entry, 0, sizeof(tss_entry));
	gdt_entry_write_base(&gdt_entries[5], (uint32_t)&tss_entry);
	gdt_entry_write_limit(&gdt_entries[5], sizeof(tss_entry)-1);
	gdt_entries[5].access = 1;
	gdt_entries[5].code = 1;
	gdt_entries[5].present = 1;
	tss_entry.ss0 = 0x08;
	tss_entry.esp0 = 0x900000;

	load_gdt(gdt);
	asm volatile(
			"mov $0x28, %%ax\n"
			"ltr %%ax\n" ::: "ax"
			);

	kalloc_init();
	// Initialise display.
	vga_clear(TERM_COLOR);
	printc("----- GeckoOS v1.0 -----\n", TERM_COLOR);
	printc("Built by random people on the internet.\n", TERM_COLOR);
	printc("Use help to see available commands.\n", TERM_COLOR);
	// Setup keyboard layouts
	set_layout(LAYOUTS[0]);

	printc("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
	init_idt();
	printc("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
	irq_install();
	printc("Enabling Timer and setting it to 50Hz...\n", VGA_COLOR_LIGHT_GREY);
	timer_install();
	timer_phase(50);
	printc("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
	asm volatile("int $0x3");
	printc("Test completed!\n", VGA_COLOR_LIGHT_GREY);

	drives_init();
	kmain(); // _entry will be the initialization
}


static void hi_usermode()
{
	asm volatile (
			"movl $0x23, %%eax\n"
			"movw %%ax, %%ds\n"
			"movw %%ax, %%es\n"
			"movw %%ax, %%fs\n"
			"movw %%ax, %%gs\n"
			::: "ax"
		     );
	((uint8_t*)0xb8000)[0] = 'a';
	/*somehow triple faults*/
	asm volatile( "int $0x80" );
}

static void usermode( void (*fn)())
{
	uint32_t func = (uint32_t)fn;
	print("user mode?\n");
	print_hex((uint32_t)hi_usermode);
	print("\n");
	print_hex((uint32_t)usermode);
	print("\n");
	
	asm volatile (
			"mov $0x23, %%ax\n"     // User data segment selector (RPL=3)
			"mov %%ax, %%ds\n"
			"mov %%ax, %%es\n"
			"mov %%ax, %%fs\n"
			"mov %%ax, %%gs\n"
			"mov %%esp, %%eax\n"    // Save current stack
			"pushl $0x23\n"         // User SS
			"pushl %%eax\n"         // User ESP
			"pushfl\n"              // EFLAGS
			"pushl $0x1B\n"         // User CS
			"pushl $1f\n"            // EIP
			"iret\n"
			"1:"
			: 
			: "r"(func)
			: "eax"
		     );
	print("not user mode?\n");
	for (;;);
}

static void kmain()
{
    // malloc(938); Idk if it works tbh
    // outb(0x64, 0xfe); // Reboots the machine? (It acts weird in QEMU, but it reboots at least)
    get_kdrive(0);
    
    
    //usermode(hi_usermode);

    while (1) {    // Shell loop
        // Prints shell prompt
        printc("> ", PROMPT_COLOR);
        
        //Obtains and processes the user input

        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }

    //asm volatile ("hlt");
}

