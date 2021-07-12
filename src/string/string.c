#include "string.h"
#include <stdint.h>

int strlen(const char* str)
{
    int len = 0;
    while(str[len])
    {
        len++;
    }
    return len;
}

int strnlen(const char* str, int MAX)
{
    int i = 0;
    for(; i < MAX; i++)
    {
        if(!str[i])
            break;
    }
    return i;
}

char* strcpy(char* dest, const char* src)
{
    char* res = dest;
    while (*src != 0)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }
    *dest = 0x00;
    return res;
}

char* strncpy(char* dest, const char* src, uint32_t size)
{
    char* res = dest;
    for(int i=0; i < size - 1 && *src != 0; i++)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }
    *dest = 0x00;
    return res;
}

int isNumber(char c)
{
    return (c > 47 && c < 58) == 0;
}

int toNumericDigit(char c)
{
    return c - 48;
}

int numlen(int num)
{
    int len = 0;
    while(num){
        num /= 10;
        len++;
    }
    return len;
}


char* int_to_string(int num)
{
    char* str = "\0";
    int i = numlen(num);
    int len = i;
    while (num)
    {
        i--;
        str[i] = num % 10 + 48;
        num /= 10;
    }
    str[len] = '\0';
    return str;
}

char* int_to_hex(int num)
{
    char * str = "\0";
	int len = numlen(num);
	for(int i=0; i < len; i++)
	{
		int temp = num % 16;
		if(temp < 10)
			str[len - (i + 1)] = temp + 48;
		else
			str[len - (i + 1)] = temp + 55;
		num /= 16; 
	}
	str[len] = 'h';
	str[len+1] = '\0';
	return str;  
}

char tolower_char(char c)
{
    if(c >= 65 && c <= 90)
        c += 32;
    return c;
}

int strncmp(const char* str1, const char* str2, int max)
{
    unsigned char c1;
    unsigned char c2;
    while (max-- > 0)
    {
        c1 = (unsigned char)*str1++;
        c2 = (unsigned char)*str2++;
        if(c1 != c2)
            return c1 - c2;
        if(c1 == '\0')
            return 0;
    }
    return 0;
}

int strncmpi(const char* str1, const char* str2, int max)
{
    unsigned char c1;
    unsigned char c2;
    while (max-- > 0)
    {
        c1 = (unsigned char)*str1++;
        c2 = (unsigned char)*str2++;
        if(c1 != c2 && tolower_char(c1) != tolower_char(c2))
            return c1 - c2;
        if(c1 == '\0')
            return 0;
    }
    return 0;
}

int strlen_terminator(const char* str, int max)
{
    int i=0;
    for(; i < max; i++)
    {
        if(str[i] == '\0')
            break;
    }
    return i;
}