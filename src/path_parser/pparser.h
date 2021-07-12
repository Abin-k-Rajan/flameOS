#ifndef PPARSER_H
#define PPARSER_H

#define EBADPATH 4
#define PATH_MAX_LEN 108

struct path_root
{
    int drive_no;
    struct path_part* first_path;
};

struct path_part
{
    const char* path;
    struct path_part* next_path;
};

struct path_root* pathparser_parse(const char* path, const char* current_directory);
void pathparser_free(struct path_root* root);


#endif