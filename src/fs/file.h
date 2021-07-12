#ifndef FILE_H
#define FILE_H

#include "path_parser/pparser.h"
#include <stdint.h>

typedef unsigned int FILE_SEEK_MODE;
enum
{
    SEEK_SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int FILE_OPEN_MODE;
enum
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

enum
{
        FILE_STAT_READ_ONLY = 0b00000001
};

typedef unsigned int FILE_STAT_MODE;

struct fs_stat
{
    FILE_STAT_MODE stat;
    uint32_t file_size;
};


struct disk;
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_OPEN_MODE mode);
typedef int(*FS_RESOLVE_FUNCTION)(struct disk* disk);
typedef int(*FS_READ_FUNCTION)(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
typedef int(*FS_SEEK_FUNCTION)(void* private, uint32_t offset, FILE_SEEK_MODE whence);
typedef int(*FS_STAT_FUNCTION)(struct disk* disk ,void* private, struct fs_stat* stat);
typedef int(*FS_CLOSE_FUNCTION)(void* private);

struct file_system
{
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;
    char name[20];
};

struct file_descriptor
{
    int index;
    struct file_system* file_system;
    void* private_pointer;
    struct disk* disk;
};

void fs_init();
int fopen(const char* filename, const char* mode_string);
void fs_insert_file_system(struct file_system* file_system);
struct file_system* fs_resolve(struct disk* disk);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, uint32_t offset, FILE_SEEK_MODE whence);
int fstat(int fd, struct fs_stat* stat);
int fclose(int fd);

#endif