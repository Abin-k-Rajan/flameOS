#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "terminal/terminal.h"
#include "fat16/fat16.h"
#include "disk/disk.h"
#include "path_parser/pparser.h"
#include "string/string.h"
#include "kernel.h"

struct file_system* filesystems[FLAMEOS_MAX_FILE_SYSTEM];
struct file_descriptor* filedescriptors[FLAMEOS_MAX_FILE_DESCRIPTORS];

static struct file_system** fs_get_free_file_system()
{
    int i = 0;
    for(; i < FLAMEOS_MAX_FILE_SYSTEM; i++)
    {
        if(filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }
    return 0;
}

void fs_insert_free_file_system(struct file_system* filesystem)
{
    struct file_system** fs;
    fs = fs_get_free_file_system();
    if(fs == 0)
    {
        print("Kernel panic");
        while (1)
        {
            /* code */
        }
        
    }
    *fs = filesystem;
}

static void fs_static_load()
{
    fs_insert_free_file_system(fat16_init());
}

void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init()
{
    memset(filedescriptors, 0, sizeof(filedescriptors));
    fs_load();
}

static int file_new_descriptors(struct file_descriptor** desc_out)
{
    int res = -ENDMEM;
    for(int i=0; i < FLAMEOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if(filedescriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            desc->index = i + 1;
            filedescriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }
    return res;
}

static struct file_descriptor* file_get_descriptor(int fd)
{
    if(fd <= 0 || fd >= FLAMEOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }
    int index = fd - 1;
    return filedescriptors[index];
}

struct file_system* fs_resolve(struct disk* disk)
{
    struct file_system* fs = 0;
    for(int i=0; i < FLAMEOS_MAX_FILE_SYSTEM; i++)
    {
        if(filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }
    return fs;
}

FILE_OPEN_MODE get_mode_by_string(const char* str)
{
    FILE_OPEN_MODE mode = FILE_MODE_INVALID;
    if(strncmp(str, "r", 1) == FLAME_OS_ALL_OK)
        mode = FILE_MODE_READ;
    if(strncmp(str, "w", 1) == FLAME_OS_ALL_OK)
        mode = FILE_MODE_WRITE;
    if(strncmp(str, "a", 1) == FLAME_OS_ALL_OK)
        mode = FILE_MODE_APPEND;
    return mode;
}

int fopen(const char* filename, const char* mode_string)
{
    int res = 0;
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if(!root_path)
    {
        res = -EINVARG;
        goto out;
    }
    if(!root_path->first_path)
    {
        res = -EINVARG;
        goto out;
    }
    struct disk* disk = get_disk(root_path->drive_no);
    if(!disk)
    {
        res = -EINVARG;
        goto out;
    }
    if(!disk->file_system)
    {
        res = -EOI;
        goto out;
    }
    FILE_OPEN_MODE mode = get_mode_by_string(mode_string);
    if(mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }
    void* descriptor_private_data = disk->file_system->open(disk, root_path->first_path, mode);
    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }
    struct file_descriptor* desc = 0;
    res = file_new_descriptors(&desc);
    if(res < 0)
    {
        goto out;
    }
    desc->disk = disk;
    desc->private_pointer = descriptor_private_data;
    desc->file_system = disk->file_system;
    res = desc->index;
out:
    if(res < 0)
        res = 0;        //  CANNOT RETURN NEGATIVE VALUES IN FILE OPEN
    return res;
}




int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;
    if(size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }
    struct file_descriptor* descriptor = file_get_descriptor(fd);
    if(!descriptor)
    {
        res = -EINVARG;
        goto out;
    }
    res = descriptor->file_system->read(descriptor->disk, descriptor->private_pointer, size, nmemb, (char*)ptr);
out:
    return res;
}



int fseek(int fd, uint32_t offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* descriptor = file_get_descriptor(fd);
    if(!descriptor)
    {
        res = -EINVARG;
        goto out;
    }
    res = descriptor->file_system->seek(descriptor->private_pointer, offset, whence);
out:
    return res;
}



int fstat(int fd, struct fs_stat* stat)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EINVARG;
        goto out;
    }
    res = desc->file_system->stat(desc->disk, desc->private_pointer, stat);
out:
    return res;
}

int fclose(int fd)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if(!desc)
    {
        res = -EOI;
        goto out;
    }
    res = desc->file_system->close(desc->private_pointer);
    if(res == FLAME_OS_ALL_OK)
    {
        filedescriptors[desc->index - 1] = 0x00;
        kfree(desc);
    }
out:
    return res;
}