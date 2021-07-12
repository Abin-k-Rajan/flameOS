#include "kernel.h" 
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include <stdint.h>
#include <stddef.h>
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "terminal/terminal.h"
#include "string/string.h"
#include "path_parser/pparser.h"
#include "disk/disk_streamer.h"
#include "fs/file.h"
#include "fs/fat16/fat_helper.h"
#include "status.h"
#include "config.h"
#include "gdt/gdt.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"


extern void problem();

static struct paging_4gb_chunk* kernel_chunk = 0;

void panic(const char* msg)
{
	println((char*)msg);
	while (1)
	{
		/* code */
	}
	
}

struct tss tss;
struct gdt gdt_real[FLAMEOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured structured_gdt[FLAMEOS_TOTAL_GDT_SEGMENTS] = {
	{.base = 0x00, .limit = 0x00, .type = 0x00},
	{.base = 0x00, .limit = 0xffffffff, .type = 0x9a},
	{.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}    
};

void kernel_main()
{
	terminal_initialize();
	print_string("Welcome to flame OS\n", 10);

	memset(gdt_real, 0x00, sizeof(gdt_real));
	gdt_structures_to_gdb(gdt_real, structured_gdt, FLAMEOS_TOTAL_GDT_SEGMENTS);
	gdt_load(gdt_real, sizeof(gdt_real));

	kheap_init();

	fs_init();
	
	search_and_init();	
	idt_init();

	memset(&tss, 0x00, sizeof(tss));
	tss.esp0 = 0x600000;
	tss.ss0 = KERNEL_DATA_SEG;

	tss_load(0x28);

	kernel_chunk = paging_new_4gb(PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
	paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));

	//char* ptr = kzalloc(4096);
	//paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_IS_WRITABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
	enable_paging();


	//enable_interrupt();

	// int fd = fopen("0:/BLANK.BIN", "r");
	// if(fd)
	// {
	// 	println("File opened!");
	// }	
	// else
	// {
	// 	printerror("File can't be opened.");
	// }
	// struct fs_stat* stat = kzalloc(sizeof(struct fs_stat));
	// fstat(fd, stat);

	// char buff[stat->file_size];
	// fread(buff, stat->file_size, 1, fd);
	// buff[stat->file_size] = 0x00;
	// println(buff);

	// fclose(fd);

	struct process* process = 0;
	int fd = process_load("0:/BLANK.BIN", &process);

	if(fd != FLAME_OS_ALL_OK)
	{
		panic("Couldn't load process");
	}

	task_run_first_ever_task();	

	print("Good job agent");

}
