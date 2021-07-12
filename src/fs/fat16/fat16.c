#include "status.h"
#include "string/string.h"
#include "fat16.h"
#include "fat16_structure.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "fat_helper.h"
#include "kernel.h"
#include "terminal/terminal.h"


void* fat16_open(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
int fat16_resolve(struct disk* disk);
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* ptr);
int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE file_mode);
int fat16_fs_stat(struct disk* disk, void* private, struct fs_stat* stat);
int fat16_close(void* private);


//  THIS STRUCTURE POINTS TO ALL THE FUNCTION POINTERS IN FILE_H
//  WHEN PASSED BY ADDRESS THE FUNCTIONS ARE CALLED AUTOMATICALLY

struct file_system fat16_fs = 
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_fs_stat,
    .close = fat16_close
};

struct file_system* fat16_init()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

//  FUNCTION USED TO SET THE PRIVATE DATA ONLY FOR THE OS 
//  THIS IS NOT A FAT HEADER OR ANY OTHER ESSENTIAL INFORMATION
 
static void fat16_init_private(struct disk* disk, struct fat_private* fat_private)
{
    memset(fat_private, 0, sizeof(struct fat_private));
    fat_private->cluster_read_stream = create_disk_stream(disk->disk_id);
    fat_private->directory_stream = create_disk_stream(disk->disk_id);
    fat_private->fat_read_stream = create_disk_stream(disk->disk_id);
}

//  FUNCTION RETURNS THE ABSOLUTE ADDRESS

int fat16_directory_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

//  THIS WORKING OF THIS FUNCTION IS STILL UNCLEAR

int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;
    int res = 0;
    int i = 0;
    int directory_start = directory_start_sector * disk->sector_size;
    struct disk_streamer* stream = fat_private->directory_stream;
    if(disk_stream_set_seek(stream, directory_start) != FLAME_OS_ALL_OK)
    {
        res = -EOI;
        goto out;
    }
    while (1)
    {
        if(disk_stream_read(stream, &item, sizeof(item)) != FLAME_OS_ALL_OK)
        {
            res = -EOI;
            goto out;
        }
        //  CHECKS IF WE ARE DONE WITH THE LOOP
        if(item.filename[0] == 0x00)
        {
            break;
        }
        //  CHECKS IF THE ITEM IS UNUSED
        if(item.filename[0] == 0xE5)
        {
            continue;
        }
        i++;
    }
    res = i;   
out:
    return res;
}

//  THIS FUNCTION IS USED TO FIND AND SET THE ROOT DIRECTORY FOR OUR FAT SYSTEM

int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    int root_directory_sector_pos = (primary_header->num_of_fat_copies * primary_header->sectors_per_fat) + primary_header->reserved_sector;
    int root_directory_entries = fat_private->header.primary_header.num_possible_root_entries;
    int root_directory_size = root_directory_entries * sizeof(struct fat_directory_item);
    int total_sectors = root_directory_size / disk->sector_size;
    if(root_directory_size % disk->sector_size)
    {
        total_sectors += 1;
    }
    int total_items = fat16_get_total_items_for_directory(disk, root_directory_sector_pos);
    struct fat_directory_item* dir = kzalloc(root_directory_size);
    if(!dir)
    {
        res = -ENDMEM;
        goto out;
    }
    struct disk_streamer* stream = fat_private->directory_stream;
    if(disk_stream_set_seek(stream, fat16_directory_to_absolute(disk, root_directory_sector_pos)) != FLAME_OS_ALL_OK)
    {
        res = -EOI;
        goto out;
    }
    if(disk_stream_read(stream, dir, root_directory_size) != FLAME_OS_ALL_OK)
    {
        res = -EOI;
        goto out;
    }
    directory->item = dir;
    directory->total = total_items;
    directory->starting_sector = root_directory_sector_pos;
    directory->ending_sector_pos = root_directory_sector_pos + (root_directory_size / disk->sector_size);
out:
    return res;
}

//  THIS IS A FUNCTION POINTER THAT IS CALLED WHEN WE INITIALIZE A FILE SYSTEM
//  THIS FUNCTION IS USED TO RESOLVE A PARTICULAR FILE SYSTEM

int fat16_resolve(struct disk* disk)
{
    int res = 0;
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);    
    disk->fs_private = fat_private;
    disk->file_system = &fat16_fs;
    struct disk_streamer* stream = create_disk_stream(disk->disk_id);

    if(!stream)
    {
        res = -ENDMEM;
        goto out;
    }
    if(disk_stream_read(stream, &fat_private->header, sizeof(fat_private->header)) != FLAME_OS_ALL_OK)
    {
        res = -EOI;
        goto out;
    }
    if(fat_private->header.shared.extended_header.boot_signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }
    if(fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != FLAME_OS_ALL_OK)
    {
        res = -EOI;
        goto out;
    }

out:
    if(stream)
    {
        disk_stream_free(stream);
    }
    if(res < 0)
    {
        kfree(fat_private);
        disk->fs_private = 0;
    }
    return res;
}



void* fat16_open(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode)
{
    if(mode != FILE_MODE_READ)
    {
        return ERROR(-ERDONLY);    
    }
    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if(!descriptor)
    {
        return ERROR(-ENDMEM);
    }
    descriptor->item = fat16_get_directory_entry(disk, path);
    if(!descriptor->item)
    {
        printerror("fat16: 194");
        return ERROR(-EOI);
    }
    descriptor->pos = 0;
    return descriptor;
}


int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* ptr)
{
    int res = 0;
    struct fat_file_descriptor* fat_descriptor = descriptor;
    struct fat_directory_item* item = fat_descriptor->item->item;
    int offset = fat_descriptor->pos;
    for(uint32_t i = 0; i < nmemb; i++)
    {
        res = fat16_read_internal(disk, fat16_get_first_cluster(item), offset, size, ptr);
        if(res < 0)
        {
            goto out;
        }
        offset += size;
        ptr += size;
    }
    res = nmemb;
out:
    return res;
}



int fat16_seek(void* private, uint32_t offset, FILE_SEEK_MODE file_mode)
{
    int res = 0;
    struct fat_file_descriptor* desc = private;
    struct fat_item* item = desc->item;
    if(item->type != FAT_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }
    struct fat_directory_item* fat_item = item->item;
    if(offset >= fat_item->file_size)
    {
        res = -EOI;
        goto out;
    }
    switch (file_mode)
    {
        case SEEK_SET:
            desc->pos = offset;
            break;
        case SEEK_END:
            res = -EUNIMP;
            break;
        case SEEK_CUR:
            desc->pos += offset;
            break;
        default:
            res = -EINVARG;
            break;
    }
out:
    return res;
}


int fat16_fs_stat(struct disk* disk, void* private, struct fs_stat* stat)
{
    int res = 0;
    struct fat_file_descriptor* desc = private;
    struct fat_item* desc_item = desc->item;
    if(!desc_item)
    {
        res = -EINVARG;
        goto out;
    }
    stat->stat = 0;
    if(desc_item->item->attribute & FAT_FILE_READ_ONLY)
    {
        stat->stat = 1;
    }
    stat->file_size = desc_item->item->file_size;
out:
    return res;
}


int fat16_close(void* private)
{
    struct fat_file_descriptor* desc = private;
    fat16_item_free(desc->item);
    kfree(desc);
    return 0;
}