org 0x7c00
BITS 16

CODE_SEG equ gdt_code-gdt_start
DATA_SEG equ gdt_data-gdt_start

jmp short start
nop

OEMIDENTIFIER	 		db 'FLAMEOS '
BYTESPERSECTOR	 		dw 0x200
SECTORPERCLUSTER 		db 0x80
RESERVEDSECTORS	 		dw 200
FATCOPIES		 		db 0x02
ROOTDIRECTORYENTRIES	dw 0x40
NUMSECTORS				dw 0x00
MEDIATYPE				db 0xF8
SECTORSPERFAT			dw 0x100
SECTORSPERTRACK			dw 0x20
NUMBEROFHEADS			dw 0x40
HIDDENSECTORS			dd 0x00
SECTORSBIG				dd 0x773594
;	_extended BPB (dos 4.0)
DRIVENUMBERS			db 0x80
WINBIT 					db 0x00
BOOTSIGNATURE			db 0x29
COLUMNID				dd 0xD105
VOLUMEIDSTRING			db 'FLAMEOS BOO'
SYSTEMIDSTRING			db 'FAT16   '

start:
	jmp 0:step2


step2:
	cli	; clear interrupts
	mov ax, 0x00
	mov es, ax
	mov ds, ax
	mov ss, ax
	mov sp, 0x7c0
	sti	; enable interrupts

.load_protected:
	cli
	lgdt[gdt_descriptor]
	mov eax, cr0
	or eax, 0x1
	mov cr0, eax
	jmp CODE_SEG:load32

;GDT

gdt_start:

gdt_null:
	dd 0x0
	dd 0x0

gdt_code:
	dw 0xffff
	dw 0
	db 0
	db 0x9a
	db 11001111b
	db 0

;offset 0x10
gdt_data:
	dw 0xffff
	dw 0
	db 0
	db 0x92
	db 11001111b
	db 0

gdt_end:

gdt_descriptor:
	dw gdt_end - gdt_start - 1
	dd gdt_start

[BITS 32]
load32:
	mov eax, 1	;starting sector
	mov ecx, 100	;total sectors
	mov edi, 0x0100000
	call ata_lba_read
	jmp CODE_SEG:0x0100000

ata_lba_read:
	mov ebx, eax

	;send the highest 8 bits
	shr eax, 24
	or eax, 0xE0	;selects the master drive
	mov dx, 0x1f6
	out dx, al
	;finished sending 8 bits

	;send the total sectors to read
	mov eax, ecx
	mov dx, 0x1f2
	out dx, al
	;finished sending

	;send more bits
	mov eax, ebx
	mov dx, 0x1f3
	out dx, al
	;finished sending more bits

	mov dx, 0x1f4
	mov eax, ebx
	shr eax, 8
	out dx, al

	mov dx, 0x1f5
	mov eax, ebx
	shr eax, 16
	out dx, al

	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

.next_sector:
	push ecx

.try_again:
	mov dx, 0x1f7
	in al, dx
	test al, 8
	jz .try_again

;we read 256 bit word	
	mov ecx, 256
	mov dx, 0x1f0
	rep insw
	pop ecx
	loop .next_sector
	ret

	
	


times 510-($-$$) db 0
dw 0xAA55

