#include "io/io.h"
#include "disk.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"

struct disk disk;

int disk_read_sector(int lba, int total, void* buf)
{
    outb(0x1F6, (lba >> 24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)(lba & 0xff));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    unsigned short* ptr = (unsigned short*) buf;
    for (int b = 0; b < total; b++)
    {
        // Wait for the buffer to be ready
        char c = insb(0x1F7);
        while(!(c & 0x08))
        {
            c = insb(0x1F7);
        }

        // Copy from hard disk to memory
        for (int i = 0; i < 256; i++)
        {
            *ptr = insw(0x1F0);
            ptr++;
        }

    }
    return 0;
} 

void search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = HARD_DISK_CONFIG;
    disk.sector_size = FLAMEOS_SECTOR_SIZE;
    disk.disk_id = 0; 
    disk.file_system = fs_resolve(&disk);
}


struct disk* get_disk(int index)
{
    if(index != 0)
        return 0;
    return &disk;
}


int read_from_disk(struct disk* idisk, int lba, int sector, void* buff)
{
    if(idisk != &disk)
    {
        return -EOI;
    }
    return disk_read_sector(lba, sector, buff);
}