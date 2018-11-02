#include "tips.h"
#include <stdlib.h>
#include <time.h>

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

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

	lru.value ranges from 0 to assoc:
		-lru.value = 0 -----> this block has not been used
		-lru.value = 1 -----> this is the most recently used block
		-lru.value = assoc -> this is the least recently used block

	Note: When a block gets accessed, this function gets called on that block.
				This function will update the blocks lru.value to 1(most recently used)
				and increment this value for all the other blocks in the set by 1.
*/
void update_lru(int assoc_index, int block_index)
{
	//Update the accessed block to be the most recently used block
	cache[assoc_index].block[block_index].lru.value = 1;
	//Update the rest of the blocks in the set to reflect this change
	for(int index = 0; index < assoc; index++) {
		if(index != block_index) {
			cache[assoc_index].block[index].lru.value++;
		}
	}
}


/*
	This function chooses which block should be replaced, based on replacement policy

		set - the set from the cache that we want a block to be replaced in

	returns a pointer to the block that we want to replace

*/
cacheBlock* replacementBlock(cacheSet* set) {
	cacheBlock* block = NULL;

	switch(policy) {
		case RANDOM://Choose a random block in the set to replace
			block = &(set->block[randomint(assoc)]);
			break;
		case LRU: //The block to replace has `lru.value` == `assoc`
			//For each block in the set
			for(int block_index = 0; block_index < assoc; block_index++) {
				//Check if this block is least recently used (lru.value == assoc)
				if(set->block[block_index].lru.value == assoc) {
					block = &(set->block[block_index]);
				}
			}
			break;
		case LFU:	//The block to replace has the lowest `accessCount`
			//Initially choose the first block as the LFU block
			block = &(set->block[0]);
			//For each block in the set, beginning with the second block:
			//compare the block with the LFU block
			for(int block_index = 1; block_index < assoc; block_index++) {
				//If the block is less frequently used than the current *LFU block
				if(set->block[block_index].accessCount < block->accessCount) {
					block = &(set->block[block_index]);
				}
			}
			break;
	}

	return block;
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
	unsigned int offset_size = uint_log2(block_size/4); //Number of bits needed for the offset
	unsigned int index_size = uint_log2(set_count);			//Number of bits needed for the index

	unsigned int offset_mask = (block_size / 4) - 1;
	unsigned int index_mask = 2^index_size - 1 << offset_size;

	unsigned int offset = addr & offset_mask;
	unsigned int index = addr & index_mask;
	unsigned int tag = addr >> (index_size + offset_size);

	printf("%d tag bits\t%d index bits\t%d offset bits\n", 32 - (index_size + offset_size), index_size, offset_size);

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

	if(we == READ) {
		for(int block_index = 0; block_index < assoc; block_index++) {	//Loop through all blocks in the set
			if(cache[index].block[block_index].tag == tag) { 	//READ HIT!!
				*data = cache[index].block[block_index].data[offset];	//Put the value found in data
				//Highlight the word in GREEN
				//highlight_offset(index, block_index, offset, HIT);
				//Update replacement policy information
				if(policy == LRU)
					update_lru(index, block_index);
				else if(policy == LFU)
					cache[index].block[block_index].accessCount++;
				//No need to continue, so break out of for loop
				break;
			}
		}
		//highlight_offset(index, 0, offset, MISS);
		//At this point we looked through all the blocks in the set and none of the tags
		//match the address tag. We have a READ MISS!
		//Handle read miss here.
	}
	else {	//if (we == WRITE)
	}

  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
  accessDRAM(addr, (byte*)data, WORD_SIZE, we);
}
