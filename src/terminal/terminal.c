#include "terminal.h"
#include "string/string.h"


uint16_t terminal_row;
uint16_t terminal_col;
uint16_t* video_mem;

void terminal_put_char(char, uint8_t, uint8_t, int);
void terminal_adjust_aligment();


uint16_t terminal_make_char(char c, int color)
{
	return (color << 8) | c;
}

void terminal_initialize()
{
	uint8_t x;
	uint8_t y;
	terminal_row = 0;
	terminal_col = 0;
	video_mem = (uint16_t*)(0xB8000);
	for(y=0; y < TERMINAL_HEIGHT; y++)
	{
		for(x=0; x < TERMINAL_WIDTH; x++)
		{
			terminal_put_char(' ', x, y, 0);
		}
	}
}

void terminal_put_char(char c, uint8_t x, uint8_t y, int color)
{
	video_mem[y * TERMINAL_WIDTH + x] = terminal_make_char(c, color);
}

void print_string(char* str, int color)
{
	size_t len = strlen(str);
	for(int i = 0; i <len; i++ )
	{
		if(str[i] == '\n')
		{
			terminal_col++;
			if(terminal_col >= TERMINAL_HEIGHT)
			{
				terminal_col = 0;
				terminal_row = 0;
			}
			terminal_row = 0;
			continue;
		}
		if(str[i] == '\t')
		{
			terminal_row += 8;
			if(terminal_row >= TERMINAL_WIDTH)
			{
				terminal_row = 0;
				terminal_col++;
			}
			if(terminal_col >= TERMINAL_HEIGHT)
			{
				terminal_col = 0;
				terminal_row = 0;
			}
			continue;
		}
		terminal_put_char(str[i], terminal_row++, terminal_col, color);
        terminal_adjust_aligment();
	}
}

void terminal_adjust_aligment()
{
    if(terminal_row >= TERMINAL_WIDTH){
		terminal_col++;
		terminal_row = 0;
	}
	if(terminal_row >= TERMINAL_WIDTH && terminal_col >= TERMINAL_HEIGHT)
	{
		terminal_col = 0;
		terminal_row = 0;
	}
}


void print(char* str)
{
    print_string(str, 15);
}

void println(char* str)
{
    print(str);
    terminal_col++;
    terminal_row = 0;
    terminal_adjust_aligment();
}

void printerror(char* str)
{
    print_string(str, 4);
    terminal_col++;
    terminal_row = 0;
    terminal_adjust_aligment();
}