#ifndef STRING_H
#define STRING_H

#include <stdint.h>

int strlen(const char* str);
int strnlen(const char* str, int MAX);
int isNumber(char c);
int toNumericDigit(char c);

char* int_to_string(int num);
char* int_to_hex(int num);

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, uint32_t size);
int strlen_terminator(const char* str, int max);
int strncmpi(const char* str1, const char* str2, int max);
int strncmp(const char* str1, const char* str2, int max);
char tolower_char(char c);

#endif