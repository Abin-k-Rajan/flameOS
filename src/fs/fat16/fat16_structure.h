#ifndef FAT16_STRUCTURE_H
#define FAT16_STRUCTURE_H

#include <stdint.h>
#include "disk/disk.h"
#include "disk/disk_streamer.h"

#define FAT_SIGNATORE     0x29
#define FAT_ENTRY_SIZE    0x02
#define FAT_BAD_SECTOR    0xFF7
#define FAT_UNUSED_SECTOR 0x00

typedef unsigned int FAT_ITEM_TYPE;
#define FAT_TYPE_DIRECTORY 0
#define FAT_TYPE_FILE 1

//  BIT MASK FOR FILE MODE

#define FAT_FILE_READ_ONLY       0x01
#define FAT_FILE_HIDDEN          0x02
#define FAT_FILE_SYSTEM          0x04
#define FAT_FILE_VOLUME_LABEL    0x08
#define FAT_FILE_SUBDIRECTORY    0x10
#define FAT_FILE_ARCHIVE         0x20
#define FAT_FILE_DEVICE          0x40
#define FAT_FILE_RESERVED        0x80

struct fat_header
{
    uint8_t short_jmp_ins[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sector_per_cluster;
    uint16_t reserved_sector;
    uint8_t num_of_fat_copies;
    uint16_t num_possible_root_entries;
    uint16_t small_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t number_of_header;
    uint32_t hidden_sectors;
    uint32_t large_sectors;
}__attribute__((packed));

struct fat_header_extended
{
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t file_system_type[8];
}__attribute__((packed));

struct fat_directory_item
{
    uint8_t filename[8];
    uint8_t extension[3];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t creation_milli_stamp;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t high_16_bit_first_cluster;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t low_16_bit_first_cluster;
    uint32_t file_size;
}__attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};



struct fat_directory
{
    struct fat_directory_item* item;
    int total;
    int starting_sector;
    int ending_sector_pos;
};

struct fat_item
{
    union
    {
        struct fat_directory_item* item;
        struct fat_directory* directory;
    };
    FAT_ITEM_TYPE type;
};

struct fat_file_descriptor
{
    struct fat_item* item;
    uint32_t pos;
};

struct fat_private
{
    struct fat_h header;
    struct fat_directory root_directory;
    struct disk_streamer* cluster_read_stream;
    struct disk_streamer* fat_read_stream;
    struct disk_streamer* directory_stream; 
};

#endif