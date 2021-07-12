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
#include "terminal/terminal.h"



void fat16_to_proper_string(char** out, const char* in)
{
    while (*in != 0x00 && *in != 0x20)
    {
        **out = *in;
        *out += 1;
        in += 1;
    }
    if(*in == 0x20)
    {
        **out = 0x00;
    }
}



void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len)
{
    memset(out, 0x00, max_len);
    char* out_temp = out;
    fat16_to_proper_string(&out_temp, (const char*)item->filename);
    if(item->extension[0] != 0x00 && item->extension[0] != 0x20)
    {
        *out_temp++ = '.';
        fat16_to_proper_string(&out_temp, (const char*)item->extension);
    }
}




struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size)
{
    struct fat_directory_item* directory_item = 0;
    if(size < sizeof(struct fat_directory_item))
    {
        printerror("fat_helper, line: 54");
        return 0;
    }
    directory_item = kzalloc(size);
    if(!directory_item)
    {
        printerror("fat_helper, line: 60");
        return 0;
    }
    memcpy(directory_item, item, size);
    return item;
}




uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return item->high_16_bit_first_cluster | item->low_16_bit_first_cluster;
}



static int fat16_cluster_to_sector(struct fat_private* private, int cluster)
{
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sector_per_cluster);
}


static int fat16_get_first_fat_sector(struct fat_private* fat_private)
{
    return fat_private->header.primary_header.reserved_sector;
}


static int fat16_get_fat_entry(struct disk* disk, int cluster)
{
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_streamer* stream = private->fat_read_stream;
    if(!stream)
    {
                printerror("fat_helper, line: 96");
        goto out;
    }
    uint32_t fat_table_pos = fat16_get_first_fat_sector(private) * disk->sector_size;
    res = disk_stream_set_seek(stream, fat_table_pos * (cluster * FAT_ENTRY_SIZE));
    if(res < 0)
    {
                printerror("fat_helper, line: 103");
        goto out;
    }
    uint16_t result = 0;
    result = disk_stream_read(stream, &result, sizeof(result));
    if(result < 0)
    {
                printerror("fat_helper, line: 110");
        goto out;
    }
    res = result;
out:
    return res;
}

static int fat16_size_of_cluster_bytes(struct disk* disk)
{
    struct fat_private* fat_private = disk->fs_private;
    return fat_private->header.primary_header.sector_per_cluster * disk->sector_size;
}



static int fat16_cluster_for_offset(struct disk* disk, int starting_cluster, int offset)
{
    int res = 0;
    int size_of_cluster_bytes = fat16_size_of_cluster_bytes(disk);
    int cluster_to_use = starting_cluster;
    int cluster_ahead = offset / size_of_cluster_bytes;
    for(int i=0; i < cluster_ahead; i++)
    {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        
        //  CHECKS FOR LAST ENTRY FF8 AND FFF, REFER MANUAL FOR DETAILS
        if(entry == 0xFF8 || entry == 0xFFF)
        {
                    printerror("fat_helper, line: 139");
            res = -EOI;
            goto out;
        }

        //  CHECKS FOR BAD SECTOR, OFTEN DUE TO HARDWARE REASONS FF7 signifies this
        if(entry == 0xFF7)
        {
                    printerror("fat_helper, line: 147");
            res = -EOI;
            goto out;
        }

        //  CHECKS IF THE SECTOR IS RESERVED, FF0 OR FF6 DOES THE JOB
        if(entry == 0xFF0 || entry == 0xFF6)
        {
                    printerror("fat_helper, line: 155");
            res = -EOI;
            goto out;
        }

        //  CHECKS FOR NULL VALUE 0x00
        if(entry == 0x00)
        {
                    printerror("fat_helper, line: 163");
            res = -EOI;
            goto out;
        }
        cluster_to_use = entry;
    }
    res = cluster_to_use;
out:   
    return res;
}


static int fat16_read_internal_from_stream(struct disk* disk, struct disk_streamer* stream, int cluster, int offset, int total, void* out)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = fat16_size_of_cluster_bytes(disk);
    int cluster_to_use = fat16_cluster_for_offset(disk, cluster, offset);
    if(cluster_to_use < 0)
    {
                printerror("fat_helper, line: 183");
        res = cluster_to_use;
        goto out;
    }
    int offset_from_cluster = offset % size_of_cluster_bytes;
    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    
    res = disk_stream_set_seek(stream, starting_pos);
    if(res < 0)
    {
                printerror("fat_helper, line: 195");
        goto out;
    }

    res = disk_stream_read(stream, out, total_to_read);
    if(res < 0)
    {
                printerror("fat_helper, line: 202");
        goto out;
    }
    total -= total_to_read;

    if (total > 0)
    {
        fat16_read_internal_from_stream(disk, stream, cluster, offset + total_to_read, total, out + total_to_read);
    }
    
    
out:
    return res;
}


int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out)
{
    struct fat_private* private = disk->fs_private;
    struct disk_streamer* stream = private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}

void fat16_free_directory(struct fat_directory* directory)
{
    if(!directory)
    {
        return;
    }
    if(directory->item)
    {
        kfree(directory->item);
    }
    kfree(directory);
}


void fat16_item_free(struct fat_item* item)
{
    if(item->type == FAT_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if(item->type == FAT_TYPE_FILE)
    {
        kfree(item->item);
    }
    kfree(item);
}



struct fat_directory* fat16_load_fat_directory(struct disk* disk,struct fat_directory_item* item)
{
    struct fat_directory* directory = 0;
    int res = 0;
    struct fat_private* private = disk->fs_private;
    if(!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
        printerror("fat_helper, line: 261");
        res = -EINVARG;
        goto out;
    }
    directory = kzalloc(sizeof(struct fat_directory));
    if(!directory)
    {
        printerror("fat_helper, line: 268");
        res = -ENDMEM;
        goto out;
    }
    int cluster = fat16_get_first_cluster(item);
    int cluster_sector = fat16_cluster_to_sector(private, cluster);
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->total = total_items;
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    directory->item = kzalloc(directory_size);
    if(!directory->item)
    {
        printerror("fat_helper, line: 280");
        res = -ENDMEM;
        goto out;
    }
    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if(res != FLAME_OS_ALL_OK)
    {
        printerror("fat_helper, line: 287");
        goto out;
    }
out:
    if(res != FLAME_OS_ALL_OK)
    {
        fat16_free_directory(directory);
    }
    return directory;
}


struct fat_item* fat16_new_item_for_directory_item(struct disk* disk, struct fat_directory_item* item)
{
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if(!f_item)
    {
        printerror("fat_helper, line: 304");
        return 0;
    }
    if(item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_TYPE_DIRECTORY;
    }
    f_item->type = FAT_TYPE_FILE;
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    return f_item;
}


struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name)
{
    struct fat_item* f_item = 0;
    char temp_filename[FLAMEOS_MAX_FILE_LENGTH];
    for(int i=0; i < directory->total; i++)
    {
        fat16_get_full_relative_filename(&directory->item[i], temp_filename, sizeof(temp_filename));
        if(strncmp(temp_filename, name, sizeof(temp_filename)) == 0)
        {
            f_item = fat16_new_item_for_directory_item(disk, &directory->item[i]);
        }
    }
    return f_item;
}



struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    struct fat_private* private = disk->fs_private;
    struct fat_item* current_item = 0;
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &private->root_directory, path->path);
    if(!root_item)
    {
        printerror("fat_helper, line: 342");
        return current_item;
    }
    struct path_part* next_path = path->next_path;
    current_item = root_item;
    while (next_path != 0)
    {
        if(!(current_item->type & FAT_FILE_SUBDIRECTORY))
        {
            printerror("fat_helper, line: 351");
            current_item = 0;
            break;
        }
        struct fat_item* temp_item = fat16_find_item_in_directory(disk, current_item->directory, next_path->path);
        fat16_item_free(current_item);
        current_item = temp_item;
        next_path = next_path->next_path;
    }
    return current_item;
}