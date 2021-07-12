#ifndef KERNEL_H
#define KERNEL_H

#define TERMINAL_HEIGHT 20
#define TERMINAL_WIDTH 80

void kernel_main();
void print_string(char* str, int color);
void panic(const char* msg);

#define ISERR(value) ((int)value < 0)
#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)

#endif
