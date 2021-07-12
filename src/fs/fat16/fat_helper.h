#ifndef FAT_HELPER_H
#define FAT_HELPER_H

#include "fat_helper.h"
#include "fat16_structure.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "disk/disk_streamer.h"
#include "disk/disk.h"
#include "fat16_structure.h"
#include "fat16.h"
#include "status.h"
#include "config.h"
#include "string/string.h"
#include "path_parser/pparser.h"
#include <stdint.h>

struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path);
void fat16_free_directory(struct fat_directory* directory);
static int fat16_read_internal_from_stream(struct disk* disk, struct disk_streamer* stream, int cluster, int offset, int total, void* out);
static int fat16_cluster_for_offset(struct disk* disk, int starting_cluster, int offset);
static int fat16_size_of_cluster_bytes(struct disk* disk);
static int fat16_get_fat_entry(struct disk* disk, int cluster);
static int fat16_cluster_to_sector(struct fat_private* private, int cluster);
uint32_t fat16_get_first_cluster(struct fat_directory_item* item);
struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size);
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len);
void fat16_to_proper_string(char** out, const char* in);
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name);
struct fat_item* fat16_new_item_for_directory_item(struct disk* disk, struct fat_directory_item* item);
int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out);

void fat16_item_free(struct fat_item* item);


#endif