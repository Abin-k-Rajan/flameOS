#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

typedef unsigned int FLAMEOS_DISK_TYPE;

#define HARD_DISK_CONFIG 0

struct disk
{
    FLAMEOS_DISK_TYPE type;
    int sector_size;
    int disk_id;
    struct file_system* file_system;
    void* fs_private;
};


int read_from_disk(struct disk* idisk, int lba, int sector, void* buff);
struct disk* get_disk(int index);
void search_and_init();

#endif