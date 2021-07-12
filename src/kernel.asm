[BITS 32]
global _start
global problem
extern kernel_main
CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov ebp, 0x00200000
	mov esp, ebp

	;Enable A20 line
	in al, 0x92
	or al, 2
	out 0x92, al

	call remap_master_pic
	call kernel_main

	jmp $

remap_master_pic:
	mov al, 00010001b
	out 0x20, al	;tell master pic

	mov al, 0x20	;int 0x20 is where master pic should start
	out 0x21, al

	mov al, 0000001b
	out 0x21, al

	ret


problem:
	int 0

times 512 - ($ - $$) db 0
