#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "config.h"
#include "task.h"

struct process
{
    //  Every Process is identified with unique key called process id
    uint16_t process_id;
    //  Name of the file that process is servicing
    char file_name[FLAMEOS_MAX_FILE_LENGTH];
    //  Task for the process
    struct task* task;
    //  Track of all memory allocations within a process, so it can be cleared on termination
    void* allocations[FLAMEOS_MAX_PROGRAMS];
    //  Pointer to the process
    void* ptr;
    //  Address of user stack
    void* stack;
    //  Size of the process
    uint32_t size;
};

int process_load_for_slot(const char* filename, struct process** process, int process_slot);
int process_load(const char* filename, struct process** process);


#endif