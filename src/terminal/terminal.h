#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>
#include <stddef.h>

#define TERMINAL_HEIGHT 20
#define TERMINAL_WIDTH 80

void print_string(char* str, int color);
void terminal_initialize();

void println(char* str);
void print(char* str);


void printerror(char* str);


#endif