#include "tips.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

unsigned int uint_pow2(int n) {
	unsigned int i, r = 1;
	for(i = 0; i < n; i++) {
		r = 2*r;
	}
	return r;
}

/* return random int from 0..x-1 */
int randomint( int x );


/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
	This function updates the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

	lru.value ranges from 0 to assoc(when all the blocks in the set are VALID):
		-lru.value = 0 -----> this block has not been used
		-lru.value = 1 -----> this is the most recently used block
		-lru.value = assoc -> this is the least recently used block

	Note: When a block gets accessed, this function gets called on that block.
				This function will update the blocks lru.value to 1(most recently used)
				and increment this value for all the other blocks in the set by 1.
*/
void update_policy_info(int assoc_index, int block_index) {
	//Update the accessed block to be the most recently used block
	cache[assoc_index].block[block_index].lru.value = 1;
	//Update the rest of the blocks in the set to reflect this change
	for(int index = 0; index < assoc; index++) {
		int current_lru_value = cache[assoc_index].block[index].lru.value;
		if(cache[assoc_index].block[index].valid == VALID &&
			 cache[assoc_index].block[index].lru.value <= assoc &&
			 index != block_index) {
			cache[assoc_index].block[index].lru.value++;
		}
	}
}

/*
	This function checks if there is a block in the given set that is invalid

		set - pointer to a set needing to be checked

	returns the block index of the first invalid block found, or -1 if none found
*/
int search_for_invalid(cacheSet* set) {
	//Loop through all blocks in the set
	for(int block_index = 0; block_index < assoc; block_index++) {
		//Return the block index if the block is invalid
		if(set->block[block_index].valid == INVALID) {
			return block_index;
		}
	}

	return -1; //No invalid blocks in set
}

cacheBlock* get_lru_block(cacheSet* set, int* index) {
			cacheBlock* block = NULL;
			int block_index;

			for(block_index = 0; block_index < assoc; block_index++) {
				*index = block_index;
				printf("\tSelecting block %d...\n", *index);
				block = &(set->block[block_index]);
				printf("Testing block %d...\n", *index);

				//Check if this block is least recently used (lru.value == assoc)
				if(block->lru.value == assoc) {
					printf("This block is LRU, lru.value is %d\n", block->lru.value);
					break;
				}

			}
			return block;
}

/*
	This function chooses which block should be replaced, based on replacement policy

		set - the set from the cache that we want a block to be replaced in
		block_i - an idicator telling which block has been returned

	returns a pointer to the block that we want to replace

*/
cacheBlock* replacementBlock(cacheSet* set, unsigned int* block_i) {
	cacheBlock* block = NULL;

	//For any repacement policy, give precedence to an invalid block
	int invalid = search_for_invalid(set);
	if(invalid > -1) {
		block = &(set->block[invalid]);
		*block_i = invalid;
		return block;
	}

	switch(policy) {
		case RANDOM://Choose a random block in the set to replace
			*block_i = randomint(assoc);
			block = &(set->block[*block_i]);
			break;
		case LRU: //The block to replace has `lru.value` == `assoc`
			//TODO:FIX lru information
			//For each block in the set
			block = get_lru_block(set, block_i);
			break;
		case LFU:	//The block to replace has the lowest `accessCount`
			//Initially choose the first block as the LFU block
			*block_i = 0;
			block = &(set->block[0]);
			int count = 1; 	//Keeps track of how many block are LFU blocks
			//For each block in the set, beginning with the second block:
			//compare the block with the LFU block
			for(int block_index = 1; block_index < assoc; block_index++) {
				//If the current block is less frequently used than the current LFU block
				if(set->block[block_index].accessCount <= block->accessCount) {
					if(set->block[block_index].accessCount == block->accessCount) {
						count++;
					}
					else {
						count = 1;
					}
					//Make the current block the new LFU block
					*block_i = block_index;
					block = &(set->block[block_index]);
				}
			}
			if(count > 1) {
				block = get_lru_block(set, block_i);
			}
			break;
	}

	return block;
}

void dec2bin(int n) {
	int c, k;
  printf("0x%.8x is: ", n);
  for(c = 31; c >= 0; c--) {
    k = n >> c;
    if (k & 1)
      printf("1");
    else
      printf("0");
  }
  printf("\n");
}
/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
	unsigned int offset_size, index_size; //Holds # of bits needed for the index and the offset

	unsigned int offset_mask, index_mask;	//Holds masks to extrct offset and index information

	unsigned int offset, index, addr_tag; //Holds the offset, index, and tag values

	TransferUnit transfer_unit; //Hold the transfer unit size between cache and memory

	cacheBlock* block = NULL;						//A pointer to the block to replace, if any
	unsigned int block_i = -1;					//Index of the block to replace, if any
	int cacheMiss = 0;									//Flag that indicates cache miss

	/* Print to confirm correct ADDRESS, MASKS, TAG, INDEX, and OFFSET info *//*
	printf("****\t**********\t**********\t**********\t****\n");
	printf("%d Tag bits\t%d Index bits\t%d Offset bits\n", 32 - (index_size + offset_size), index_size, offset_size);
	printf("Address: ");	dec2bin(addr);
	printf("Index mask: ");	dec2bin(index_mask);
	printf("Offset mask: ");	dec2bin(offset_mask);
  printf("Tag: ");	dec2bin(addr_tag);
	printf("Index: ");	dec2bin(index);
	printf("Offset: ");	dec2bin(offset);		*/

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

 /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */
	offset_size = uint_log2(block_size); 		//Number of bits needed for the offset
	index_size = uint_log2(set_count);			//Number of bits needed for the index
	offset_mask = block_size - 1;
	index_mask = ((1 << index_size) - 1) << offset_size; //same as (uint_pow2(index_size) - 1) << offset_size;
	offset = addr & offset_mask;
	index = (addr & index_mask) >> offset_size;
	addr_tag = addr >> (index_size + offset_size);
	transfer_unit = uint_log2(block_size);

	int block_index;

	//This part is common between READ and WRITE, finding the correct block
	//Loop through all blocks in the set
	for(block_index = 0; block_index < assoc; block_index++) {
		block = &(cache[index].block[block_index]);
		printf("Accessing cache[%d].block[%d]...", index, block_index);
		if(block->valid == VALID && block->tag == addr_tag) { 	//Block FOUND!!
			printf("HIT!\n");

			block_i = block_index;
			//Highlight the word in GREEN
			highlight_offset(index, block_index, offset, HIT);
			//Copy the value to data for the caller
			memcpy(data, block->data + offset, 4);
			//Update lru policy information
			update_policy_info(index, block_index);
			//Increment accessCount for this block
			block->accessCount++;

			//Block found, break out of loop
		  break;
		}
		printf("MISS!\n");
	}
	if(block_index == assoc) {
		printf("No match found, ");
		cacheMiss = 1;
	}

	//This is also common to a READ and a WRITE: choosing the block to replace,
	//writing that block to memory(write-back), resetting the accessCount for that
	//block(LFU), and updating the lru information for the set, before replacing
	if(cacheMiss) {

		if(we == READ)
			printf("READ MISS!\n\n");
		else if(we == WRITE)
			printf("WRITE MISS!\n\n");

		printf("Choosing block to replace...");
		block = replacementBlock(&cache[index], &block_i);
		printf("Selected block: %d\n", block_i);

		//Outline the block and highlight the offset
		highlight_block(index, block_i);
		highlight_offset(index, block_i, offset, MISS);
	  printf("Highlighted cache[%d].block[%d].data[%d]\n", index, block_i, offset);

		//Check if the block needs to be written to DRAM first (for write-back)
		if(memory_sync_policy == WRITE_BACK && block->dirty == DIRTY) {
			//Get the address for this block from the blocks tag and the index of the set.
			unsigned int block_addr = (block->tag << (index_size + offset_size)) + (index << offset_size);
			printf("Block needs to be written to memory address 0x%.8x before replacing.\nAccessing DRAM...\n", block_addr);
			accessDRAM(block_addr, block->data, transfer_unit, WRITE);
			printf("Write successful from cache[%d].block[%d] to memory address 0x%.8x\n", index, block_i, block_addr);
		}

		//Handle the miss by moving the block from DRAM into this block
		printf("Getting replacement block from memory address %.8x...\n\n", addr);
		//Put block found at memory location into the replaced blocks data field
		accessDRAM(addr, block->data, transfer_unit, READ);
		//Update LRU policy information
		update_policy_info(index, block_i);
		//Set this block as VALID
		block->valid = VALID;
		//Update the tag field
		block->tag = addr_tag;
		//Reset accessCount;
		if(policy == LFU)
			block->accessCount = 0;
		//Set this block as VIRGIN
		if(memory_sync_policy == WRITE_BACK)
			block->dirty = VIRGIN;

	}//End if(cacheMiss)

	for(int i  = 0; i < block_size; i++) {
		printf("%.2x ", block->data[i]);
	}
	printf("\n");


	if(we == READ) {
		//If read, copy the wanted byte from cache into `data` for the caller
		printf("Reading data from cache[%d].block[%d].data[%d]\n", index, block_i, offset);
		memcpy(data, block->data + offset, 4);
	}	else {
		//If write, copy the wanted byte from `data` into cache
		printf("Writing data to cache[%d].block[%d].data[%d]\n", index, block_i, offset);
		memcpy(block->data + offset, data, 4);
		//This block has been modified, set the dirty bit
		block->dirty = DIRTY;
		//Update the block in memory (for write-through)
		if(memory_sync_policy == WRITE_THROUGH) {
			printf("Writing through to memory address %.8x...\n\n", addr);
			accessDRAM(addr, block->data, transfer_unit, WRITE);
		}
	}

	printf("\n****\t**********\t**********\t**********\t****\n\n");

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
//  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
