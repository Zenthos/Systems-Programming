#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mymalloc.h"
#define MAX_HEAP_SIZE (1 << 20) // One Million Bits

/* ---------------- */
/* Type Definitions */
/* ---------------- */

typedef struct block {
	int free;
	size_t size;
	void *address;

	struct block *next;
	struct block *prev;
} Block;

/* ---------------- */
/* Global Variables */
/* ---------------- */

void *heap;
Block* next_fit_ptr = NULL;
int allocMethod = 0;

/* ------------------------- */
/* Malloc Allocation Methods */
/* ------------------------- */

Block* first_fit(size_t size) {
	Block *head = (Block*)heap;

	// Find the first open block in heap
	while (head != NULL) {
		if (head->free == 0 && head->size >= size) {
			return head;
		}

		head = head->next;
	}

	// return NULL if could not find free block
	return NULL;
}

Block* next_fit(size_t size) {
	// Set the next fit ptr if first time running
	if (next_fit_ptr == NULL) {
		next_fit_ptr = first_fit(size);
		return next_fit_ptr;
	}

	Block *head = next_fit_ptr;

	// Search for the next open block, starting from previous ptr
	while (head != NULL) {
		if (head->free == 0 && head->size >= size) {
			return head;
		}

		head = head->next;
	}

	// return NULL if could not find free block
	return NULL;
}

Block* best_fit(size_t size) {
	Block *head = (Block*)heap;
	Block *best_block = NULL;

	// Loop through all blocks to find the best block
	while (head != NULL) {
		if (head->free == 0 && head->size >= size) {
			// Set the best block if we find a smaller open block
			if (best_block == NULL) {
				best_block = head;
			} else {
				if (head->size < best_block->size) {
					best_block = head;
				}
			}
		}
		head = head->next;
	}

	return best_block;
}

/* ------------------ */
/* Coalescing Methods */
/* ------------------ */

void coalesce_prev(Block *head) {
	head->prev->size += head->size;

	if (head->next != NULL) {
		head->next->prev = head->prev;
	}

	head->prev->next = head->next;
}

void coalesce_next(Block *head) {
	head->size += head->next->size;
	head->next = head->next->next;

	if (head->next != NULL) {
		head->next->prev = head;
	}
}

/* ------------------------- */
/* My Malloc Library Methods */
/* ------------------------- */

void myinit(int allocAlg) {
	// Set the allocation method of mymalloc
	// 0 = First-Fit
	// 1 = Next-Fit
	// 2 = Best-Fit
	allocMethod = allocAlg;

	// Initialize the heap
	heap = malloc(MAX_HEAP_SIZE);

	// Initialize the explicit list
	Block header;
  	header.next = NULL;
    	header.prev = NULL;

      	// Offset the data pointer from the header block
      	header.address = heap + sizeof(Block);

        // Set the initial size to be the size of the heap minus the sizeof a single block
	header.size = MAX_HEAP_SIZE - sizeof(Block);

	// Mark the header block as free
	header.free = 0;

	// Set the first block on the heap
	memcpy(heap, &header, sizeof(Block));
}

void* mymalloc(size_t size) {
	// User attempted to allocate more memory than available
	if (size > MAX_HEAP_SIZE) {
		printf("Cannot malloc more than the max heap size\n");
		return NULL;
	}

	// Grab a free block depending on the allocation method
	// NOTE: The free_block should always be greater than or equal to the requested size
	Block *free_block = NULL;
	if (allocMethod == 0)
	{
		free_block = first_fit(size);
	}
	else if (allocMethod == 1)
	{
		free_block = next_fit(size);
	}
	else
	{
		free_block = best_fit(size);
	}

	// If no free block found, return null
	if (free_block == NULL) {
		printf("No open memory block available for the malloc request.\n");
		return NULL;
	}

	// Return the address of block, if size matches request
	if (free_block->size == size) {
		// Mark the block as not free
		free_block->free = 1;
		return free_block->address;
	}

	// If size of free block does not match request, split the block and add a header
	if (free_block->size > size) {
		// Ensure size of new block metadata and requested memory < total free block memory
		if (free_block->size > (size + sizeof(Block))) {
			// Create the new block
			Block new_block;
			new_block.next = free_block->next;
			new_block.prev = free_block;
			new_block.free = 0;

			// Set the address of the new block
			new_block.address = free_block + sizeof(Block);

			// Set the size of the new block
			new_block.size = free_block->size - size - sizeof(Block);

			// Add the new block to the heap
			memcpy(free_block + size, &new_block, sizeof(Block));

			// Update free_block
			free_block->next = free_block + size;
			free_block->size = size;
			free_block->free = 1;

			return free_block->address;
		} else {
			// If free block is not big enough to fit header block + requested memory
			printf("not enough memory to allocate.\n");
			return NULL;
		}
	}

	printf("Error, Free block found is less than requested size.\n");
	return NULL;
}

void myfree(void *ptr) {
	Block *head = (Block *)heap;

	// Do nothing on NULL pointer
	if (ptr == NULL) return;

	// Loop through the heap
	while (head != NULL) {
		if (head->address == ptr) {
			if (head->free == 0) {
				printf("error, double free\n");
				return;
			}

			// Set the block to freed
			head->free = 0;

			// Coalescing - Merge the right block if free
			if (head->next != NULL && head->next->free == 0) {
				coalesce_next(head);
			}

			// Coalescing - Merge the left block if free
			if (head->next != NULL && head->next->free == 0) {
				coalesce_prev(head);
			}

			return;
		}

		head = head->next;
	}

	printf("error, not a heap pointer\n");
}

void* myrealloc(void *ptr, size_t size) {
	Block *head = (Block *)heap;

	// Do nothing on NULL pointer and size is not 0
	if (ptr == NULL && size > 0) return NULL;

	// Do nothing on NULL pointer and size is 0
	if (ptr == NULL && size == 0) return NULL;

	// Free the allocated memory if pointer is valid, and size is 0
	if (ptr != NULL && size == 0) {
		myfree(ptr);
		return NULL;
	}

	// Find the block associated with pointer, in the heap
	while (head != NULL) {
		if (head->address == ptr) {
			int adjacent_mem = head->size;
			int prev_flag = 0;
			int next_flag = 0;

			// Determine if next block is free, and amount of memory available
			if (head->next != NULL && head->next->free == 0) {
				adjacent_mem += head->next->size;
				next_flag = 1;
			}

			// Determine if prev block is free, and amount of memory available
			if (head->prev != NULL && head->prev->free == 0) {
				adjacent_mem += head->prev->size;
				prev_flag = 1;
			}

			// If adjacement memory is large enough, reallocate in place
			if (adjacent_mem >= size)
			{
				// Coalesce prev block only
				if (prev_flag == 1 && next_flag == 0) {
					coalesce_prev(head);
				}

				// Coalesce next block only
				if (prev_flag == 0 && next_flag == 1) {
					coalesce_next(head);
				}

				// Coalesce both prev and next blocks
				if (prev_flag == 1 && next_flag == 1) {
					coalesce_next(head);
					coalesce_prev(head);
				}

				return head->address;
			}
			// Otherwise search the heap for a block with enough space
			else
			{
				void *new_ptr = mymalloc(size);

				// Move to the new block, and clear the old one
				if (new_ptr != NULL)
			       	{
					new_ptr = head->address;
					free(head->address);
				}
				// Return NULL if no free block found
				else
				{
					return NULL;
				}
			}

			return NULL;
		}

		head = head->next;
	}

	printf("error, not a heap pointer\n");
	return NULL;
}

void mycleanup() {
	free(heap);
	allocMethod = 0;	
}

double utilization() {
	double mem_in_use = 0;
	double space_in_use = 0;
	Block *head = (Block *)heap;

	while (head != NULL) {
		if (head->free != 0) {
			mem_in_use += head->size;

			double power = 1;
			while (power < head->size) {
				power *= 2;
			}

			space_in_use += power;
		}

		head = head->next;
	}

	return mem_in_use/space_in_use;
}
