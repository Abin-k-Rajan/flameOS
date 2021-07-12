#ifndef FAT16_H
#define FAT16_H
#include "fs/file.h"
#include <stdint.h>

struct file_system* fat16_init();
int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector);

#endif