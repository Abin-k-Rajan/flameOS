#include "disk_streamer.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "config.h"
#include "disk.h"


// THIS PROGRAM IS USED TO REDUCE THE LOAD OF READING FROM DISK
// USING THIS CODE WE CAN READ FROM SECTOR IN TERMS OF BYTES
// RATHER THAN USING SECTORS.

// FIRST FUNCTION IS USED TO INITIALIZE A STRUCTURE
// THAT KEEPS POSITION AND DISK ID

struct disk_streamer* create_disk_stream(int disk_id)
{
    struct disk* disk = get_disk(disk_id);
    if(!disk)
    {
        return 0x00;
    }
    struct disk_streamer* stream = kzalloc(sizeof(struct disk_streamer));
    stream->pos  = 0x00;
    stream->disk = disk;
    return stream;
}


// THIS FUNCTION IS USED TO MANUALLY SET THE SEEK OR POSITION OF STRUCTURE
// TO HELP READ FROM THAT POSITION


int disk_stream_set_seek(struct disk_streamer* stream, int pos)
{
    stream->pos = pos;
    return 0;
}


// FUNCTION USED TO FREE MEMORY OCCUPIED BY THE STRUCTURE

void disk_stream_free(struct disk_streamer* stream)
{
    kfree(stream);
}

// THIS IS HIGH LEVVEL FUNCTION THAT IS USED TO READ IN TERMS OF BYTES
// THIS FUNCTION IMPLEMENTS ALL THE ABOVE FUNCTIONS
// 
// PLEASE OPTIMISE THIS PIECE OF CODE AS IT USES RECURSSION 
// THEREFORE IT IS POSSIBLE TO RUN OUT OF STACK SPACE WHICH CAN BE DANGEROUS


int disk_stream_read(struct disk_streamer* stream, void* buff, int total)
{
    int sector = stream->pos / FLAMEOS_SECTOR_SIZE;
    int offset = stream->pos % FLAMEOS_SECTOR_SIZE;
    char buffer[FLAMEOS_SECTOR_SIZE];
    int res =  read_from_disk(stream->disk, sector, 1, buffer);
    if(res < 0)
    {
        goto out;
    }
    int total_to_read = total > FLAMEOS_SECTOR_SIZE ? FLAMEOS_SECTOR_SIZE : total;
    for(int i = 0; i < total_to_read; i++)
    {
        *(char*)buff++ = buffer[offset+i];
    }
    stream->pos += total_to_read;
    if(total > FLAMEOS_SECTOR_SIZE)
    {
        res = disk_stream_read(stream, buff, total - FLAMEOS_SECTOR_SIZE);
    }
out:
    return res;
}
