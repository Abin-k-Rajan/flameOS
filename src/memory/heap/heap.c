#include "heap.h"
#include "kernel.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"


//Does this table know how many blocks
static int heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
	int res = 0;
	size_t table_size = (size_t)(end - ptr);
	size_t total_blocks = table_size / FLMAEOS_HEAP_BLOCK_SIZE;
	if(table->total != total_blocks)
	{
		res = -EINVARG;
		print_string("heap_validate_table failed\n", 4);
		goto out;
	}
out:
	return res;
}

static int head_validate_alignment(void* ptr)
{
	return ((unsigned int)ptr % FLMAEOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
	int res = 0;
	if(!head_validate_alignment(ptr) || !head_validate_alignment(end))
	{
		res = -EINVARG;
		print_string("heap_create\n",4);
		goto out;
	}
	memset(heap, 0, sizeof(struct heap));
	heap->saddr = ptr;
	heap->table = table;

	res = heap_validate_table(ptr, end, table);
	if(res < 0)
	{
		goto out;
	}

	size_t table_size = sizeof(HEAP_BLOCK_ENTRY_TAKEN) * table->total;
	memset(table->entries, HEAP_BLOCK_ENTRY_FREE, table_size);
	
out:
	return res;
}

static uint32_t heap_alignment_value_to_upper(uint32_t val)
{
	if((val % FLMAEOS_HEAP_BLOCK_SIZE) == 0)
	{
		return val;
	}
	val = (val - (val % FLMAEOS_HEAP_BLOCK_SIZE));
	val += FLMAEOS_HEAP_BLOCK_SIZE;
	return val;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
	return entry & 0x0f;
}


int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
	struct heap_table* table = heap->table;
	int bc = 0;			//current block
	int bs = -1;		//empty block
	for(size_t i=0; i < table->total; i++)
	{
		if(heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_ENTRY_FREE)
		{
			bc = 0;
			bs = -1;
			continue;
		}
		// If this is the first block
		if (bs == -1)
		{
			bs = i;
		}
		bc++;
		if(bc == total_blocks)
		{
			break;
		}
	}
	if(bs == -1)
	{
		return -ENDMEM;
		print_string("heap heap_get_start_block\n", 4);
	}
	return bs;
}


void* heap_block_to_address(struct heap* heap, uint32_t block)
{
	return heap->saddr + (block * FLMAEOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
	int end_block = (start_block + total_blocks) - 1;
	HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
	if(total_blocks > 1)
	{
		entry |= HEAP_BLOCK_HAS_NEXT;
	}
	for(int i= start_block; i<=end_block; i++)
	{
		heap->table->entries[i] = entry;
		entry = HEAP_BLOCK_ENTRY_TAKEN;
		if(i != end_block - 1)
		{
			entry |= HEAP_BLOCK_HAS_NEXT;
		}
	}
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
	void* address = 0;
	int start_block = heap_get_start_block(heap, total_blocks);
	if(start_block < 0)
	{
		goto out;
	}
	address = heap_block_to_address(heap, start_block);
	heap_mark_blocks_taken(heap, start_block, total_blocks);
out:
	return address;
}

void* heap_malloc(struct heap* heap, size_t size)
{
	size_t aligned_size = heap_alignment_value_to_upper(size);
	uint32_t total_blocks = aligned_size / FLMAEOS_HEAP_BLOCK_SIZE;
	return heap_malloc_blocks(heap, total_blocks);
}

int heap_adders_to_block(struct heap* heap, void* address)
{
	return((int)(address - heap->saddr)) / FLMAEOS_HEAP_BLOCK_SIZE;
}

void heap_mark_blocks_free(struct heap* heap, int start_block)
{
	struct heap_table* table = heap->table;
	for(uint32_t i=start_block; i < table->total; i++)
	{
		HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
		table->entries[i] = HEAP_BLOCK_ENTRY_FREE;
		if (!(entry & HEAP_BLOCK_HAS_NEXT))
		{
			break;
		}
	}
}

void heap_free(struct heap* heap, void* ptr)
{
	heap_mark_blocks_free(heap, heap_adders_to_block(heap, ptr));
}
