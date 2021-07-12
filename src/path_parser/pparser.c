#include "pparser.h"
#include "memory/memory.h"
#include "string/string.h"
#include "memory/heap/kheap.h"
#include "string/string.h"


// THIS FUNCTION CHECKS FOR VALID PATH THAT IS LENGTH SHOULD ATLEAST BE 3 EG: "0:/"
// FURTHER CHECKS FOR DRIVE NUMBER AND CHECKS IF ":/" EXISTS

static int pathparser_path_valid_format(const char* filename)
{
    int len = strnlen(filename, PATH_MAX_LEN);
    return (len >= 3 && isNumber(filename[0]) && memcmp((void*)&filename[1], ":/", 2)) == 0;
}

//  THIS FUNCTION GETS THE DRIVE NUMBER FROM THE PATH

int pathparser_get_drive_path(const char** path)
{
    if(!pathparser_path_valid_format(*path))
    {
        return -EBADPATH;
    }
    int drive_number = toNumericDigit(*path[0]);
    *path += 3;
    return drive_number;
}

// THIS FUNCTION CREATES THE ROOT STRUCTURE THAT POINTS TO THE NEXT STRUCTURE OF THE PATH
// ALSO KEEPS THE DRIVE NUMBER

static struct path_root* pathparser_create_root(int drive_number)
{
    struct path_root* root = kzalloc(sizeof(struct path_root));
    root->drive_no = drive_number;
    root->first_path = 0x00;
    return root;
}

// THIS FUNCTION IS USED TO RIP OUT PATH INTO SEPERATE DIRECTORY NAMES
// EG: "0:/FLAMEOS/TERMINAL" -> FLAMEOS TERMINAL

static const char* pathparser_get_path_part(const char** path)
{
    char* result_path_part = kzalloc(PATH_MAX_LEN);
    int i = 0;
    while (**path != '/' && **path != 0x00)
    {
        result_path_part[i] = **path;
        *path += 1;
        i++;
    }

    if(**path == '/')
    {
        *path += 1;
    }

    if(i == 0)
    {
        kfree(result_path_part);
        result_path_part = 0;
    }
    return result_path_part;
}

// THIS FUNCTION PARTS THE PATH AND ASSOCIATES WITH THE STRUCTURE AND POINTS TO THE NEXT ONE

struct path_part* pathparser_parse_path_part(struct path_part* last_path, const char** path_name)
{
    const char* directory_name = pathparser_get_path_part(path_name);
    if(!directory_name)
    {
        return 0;
    }
    struct path_part* path_new = kzalloc(sizeof(struct path_part));
    path_new->path = directory_name;
    path_new->next_path = 0x00;
    if(last_path)
    {
        last_path->next_path = path_new;
    }
    return path_new;
}

// FUNCTION USED TO FREE PATH

void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first_path;
    while (part)
    {
        struct path_part* new_part = part->next_path;
        kfree((void*)part->path);
        kfree(part);
        part = new_part;
    }
    kfree(root);
}

// HIGH LEVEL FUNCTION THAT IMPLEMENTS ALL THE ABOVE FUNCTIONS

struct path_root* pathparser_parse(const char* path, const char* current_directory)
{
    int res = 0;
    struct path_root* root_path = kzalloc(sizeof(struct path_root));
    const char* temp_path = path;
    if(strlen(path) > PATH_MAX_LEN)
    {
        goto out;
    }
    res = pathparser_get_drive_path(&temp_path);
    if(res < 0)
    {
        goto out;
    }
    root_path = pathparser_create_root(res);
    if(!root_path)
    {
        goto out;
    }
    struct path_part* first_path = pathparser_parse_path_part(NULL, &temp_path);
    if(!first_path)
    {
        goto out;
    }
    root_path->first_path = first_path;
    struct path_part* part = pathparser_parse_path_part(first_path, &temp_path);
    while (part)
    {
        part = pathparser_parse_path_part(part, &temp_path);
    }
out:
    return root_path;
}