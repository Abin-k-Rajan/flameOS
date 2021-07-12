#ifndef STREAMER_H
#define STREAMER_H

#include "disk.h"


struct disk_streamer
{
    int pos;
    struct disk* disk;
};

struct disk_streamer* create_disk_stream(int disk_id);
int disk_stream_set_seek(struct disk_streamer* stream, int pos);
void disk_stream_free(struct disk_streamer* stream);
int disk_stream_read(struct disk_streamer* stream, void* buff, int total);

#endif