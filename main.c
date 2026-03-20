//linux :

    // If you contribute add your name here:
    //Contributors : 
                //"Ember2819"
                //    "Sifi11"
                //     "Crim"
                //     "your name here"
                //     "TheGirl790" 
                //     "Nny/CheeseFunnel23
		// kotofyt
                //	"bonk enjoyer (dorito girl)"
                // KaleidoscopeOld5841
// We share only a single file but we need multiple things to run the stuff
// Rename this file to kernel.c
// Copy over the file names, their contents and run make

/* boot.s:
;
; bonk enjoyer (dorito girl)
;
; Organize the kernel at 0x1000 for this to work properly
;
; Created, because grub will probably be too complex for the "owner" of this repo
;
;

org 0x7c00
bits 16

_start:

boot_1:
	mov	ax, 0x0
	mov	ds, ax

	mov	[BOOT_DRIVE], dl	;; On boot, DL is the drive we booted from 

	mov	si, INIT_MSG
	call	print			;; Init msg display

	mov	si, PROMPT_MSG		;; Prompt the user
	call	print

disk_prompt:
	call	wfinput			;; Wait for input

process_prompt:
	;; Display the character
	mov	ah, 0xE			
	int	0x10


	cmp	al, '1'			;; This drive
	je	al_1
	cmp	al, '2'			;; Next drive
	je	al_2
	cmp	al, '3'			;; 0x80
	je	al_3
	
	;; If neither of the inputs is defined behaviour, do the prompt again.
	jmp	disk_prompt

al_1:
	mov	dl, [BOOT_DRIVE]	;; DL = BOOT_DRIVE
	jmp	boot_2
al_2:
	mov	dl, [BOOT_DRIVE]	;; DL = BOOT_DRIVE + 1
	add	dl, 0x1
	jmp	boot_2
al_3:
	mov	dl, 0x80		;; DL = 0x80 (128)
boot_2:
	mov	si, DISK_MSG
	call	print

	mov	si, 0x0
	mov	es, si
	mov	ebx, 0x1000		;; Destination = 0x0:0x1000

	mov	al, 0x20		;; SECTOR COUNT
	mov	ch, 0x0			;; CYLINDER
	mov	cl, 0x2			;; SECTOR (STARTS WITH 1)
	mov	dh, 0x0			;; HEAD
	;; Drive already defined

	mov	ah, 0x2			;; FUNC MODE (READ SECTORS FROM DRIVE)
	int	0x13			;; INTERRUPTING BIOS

	jc	disk_error		;; If 13h carries flag, trigger a disk error

	mov	si, PROTECTED_MSG
	call	print			;; Entering protected mode msg


	;; Protected mode

	cli
	lgdt	[gdt_descriptor]

	mov	eax, cr0		;; Enabling it, duh
	or	eax, 1
	mov	cr0, eax

	;; Stack

	mov	esp, 0x900000

	jmp	0x8:protected_mode_exec	;; Executing the kernel, finally


disk_error:
	;; AH - Error code

	mov	si, DISK_ERR_MSG	;; Disk error message
	call	print

	xor	al, al
	mov	si, ax
	call	itos
	call	print			;; Error code

	mov	si, NEWLINE		;; Newline
	call	print
	
	hlt				;; Halt the bootloader

; SI - string to display.

print:
	lodsb				;; Load the next byte from SI into AL
	cmp	al, 0x0			;; Are we done?
	je	.done		
	mov	ah, 0xE 		;; BIOS Interrupt function code (0xE - display text(via VGA text mode))
	int	0x10			;; Interrupting BIOS
	jmp	print			;; loop
.done:
	ret

;; Wait for input(1 character), AL - CHARACTER
wfinput:
	mov	ah, 0x0			;; BIOS interrupt func code
	int	0x16			;; BIOS interrupt
	cmp	al, 0x0			;; If AL is 0x0, there is no input
	je	wfinput			;; Retry if no input
	ret

;; Integer to string (for debugging purposes)
;; SI - Integer, DI - String
itos:
	;; DI is going backwards btw

	mov	bx, 10			;; Decimal counting system
	mov	byte [di], 0		;; Adding null terminator
	dec	di
.loop:
	xor	dx, dx			;; Clearing the remainder of the last division
	mov	ax, si			;; setting quotient to SI
	div	bx			;; Dividing

	;;		DX:AX - dividend
	;;		AX - quotient
	;;		DX - remainder


	add	dl, '0'			;; Translating the remainder into an ASCII character
	mov	[di], dl		;; Adding the character
	dec	di			;; Moving to the next character

	mov	si, ax			;; Setting SI to the quotient of the division
	test	si, si			;; Is SI equal to 0?
	jnz	.loop			;; If it isn't, continue

	;; It is 0

	inc	di			;; Move the index up
	ret

;; Messages

INIT_MSG	db "[Boot]: Hello, World!", 0xD, 0xA, 0x0
PROTECTED_MSG	db "[Boot]: Entering protected mode.", 0xD, 0xA, 0x0
DISK_ERR_MSG	db "[Boot]: Couldn't read disk: ", 0x0
DISK_MSG	db "[Boot]: Attempting to read drive.", 0xD, 0xA, 0x0
PROMPT_MSG	db "What drive to boot from? (1 - This drive, 2 - Next drive, 3 - 0x80)", 0xD, 0xA, 0x0
NEWLINE		db 0xA, 0xD, 0x0

;; Data

BOOT_DRIVE	db 0x0

;; GDT

gdt_start:
gdt_null:
	dq	0x0
gdt_code:
	dw	0xFFFF
	dw	0x0
	db	0x0
	db	10011010b
	db	11001111b
	db	0x0
gdt_data:
	dw	0xFFFF
	dw	0x0
	db	0x0
	db	10010010b
	db	11001111b
	db	0x0
gdt_end:
gdt_descriptor:
	dw	gdt_end - gdt_start - 1
	dd	gdt_start

bits 32

;; For executing the kernel, that is 32 bit

protected_mode_exec:
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	es, ax
	mov	ss, ax
	mov	fs, ax
	mov	gs, ax

	jmp	0x1000		;; Finally, jumping to the kernel

times 510 - ($ - $$) db 0

;; Boot signature

dw 0xAA55
*/

/* Makefile */
/*
CC=cc
LD=ld
AS=nasm
S=nasm

CC_FLAGS

ASSEMBLY_FILES=boot.s
KERNEL_FILES=kernel.c
KERNEL_FILES_OBJS=$(KERNEL_FILES:.c=.o)

all: iso

iso: 

%.o: %.c
	$(CC) $(CC_FLAGS) $< -o $@
%.o: %.s
	$(AS) $(AS_FLAGS) $< -o $@

*/

// Note from CheeseFunnel23
//
// This shit is ass bro, pissed me off so much (i know code) that i had to make it ACTUALLY work.
//
// no it does not work as an actual bootable thing. (yet)
//
// i spent way too long just rewriting a base c file that runs in the terminal, i WILL be back.
// Oh and to the creator of this:
// you cannot do this in one file. might needa just have people submit push requests or whatever
// its called to the github repo.
// 20-Mar-26, sometime after midnight, CST.


//
// Bootloader:

// btw, doesnt this have a github

typedef enum VGA_COLOR {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENDA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
  VGA_COLOR_LIGHT_YELLOW = 0xE,
  VGA_COLOR_YELLOW = 0x2C
}

int puts(char* str)
{
	volatile char* pVgaTextBuff = 0xB8000;
	for (str; *str != 0; str++ )
        {
        	pVgaTextBuff[0] = *str;
        	pVgaTextBuff[1] = 0x0f; //color atrib
        	pVgaTextBuff += 2;
        }
}


void _entry()
{
	puts("Fuck your shitty slop project and learn to code without using AI like a fat gimp"); //That's rude.
}
// Notes:
// guys this is supposed to be a real project cmon 
// -cheesefunnel23

// kotofyt:
// can confirm it
// it should be ran through grub i think
// we may need to add here a shell script to build and run it

// bonkenjoyer(doritogirl):
// ^^^ YOU CAN ADD MULTIPLE FILES IN HERE?
// ^ you physically cannot a kernel in a single file, codeshare is useless here
// Anyways, im writing a bootloader in ASM right now.
// ^ im about 149 lines in
// IM done with the bootloader. it workss with my kernel. im gonna add it here rq

// wtf is this
// but hey atleast its not ai slop
// -benj (u/EastConsequence3792)
// I use Arch btw (u/Kaleidoscope5841)
// ^ used to be:(

// Uploaded to Github 10 : 00pm march 19 Saved Progress : https://github.com/Ember2819/Random-People-Coding-Stuff
