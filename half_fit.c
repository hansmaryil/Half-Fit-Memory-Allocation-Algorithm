#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include "half_fit.h"
#include "type.h"

//global variable declaration
half_fit_t half_fit;
char *base_address;
U32 init_size = 32768;
char *bin_ptrs[11]; //initialize an array of char pointers to point to the first block in each bin (initially NULL)

//this helper fcn initializes the bits that represent addresses of the next and prev blocks in a bucket to 0
//this ONLY works for the initial block of 32768 bytes
void half_init_unallocated_block (char *address)
{
    *(address + 4) = *(address + 4) >> 8; // initialize the previous bucket ptr to 0s
    *(address + 5) = *(address + 5) >> 8; // initialize the next bucket ptr to 0s
    *(address + 6) = *(address + 6) >> 4; // complete the initialization

}

void  half_init( void )
{
    U32 i;
    base_address = (char *) malloc(init_size);
    printf("Base Address: %d %x\n", base_address, base_address);

    //initializing header (first 4 bytes)
    // both previous and next ptrs in the header are pointing to 0000000000 (pointing to itself)
    *base_address = *base_address >> 8; //set the first 8 bits to 0
    *(base_address + 1) = *(base_address + 1) >> 8;//the next 8 bits to 0
    *(base_address + 2) = *(base_address + 2) >> 8;// this completes the declaration of previous and next to itself
    //size of 1024 will be represented as 0000000000 because size can be between 1 and 1024 but 1024 requires 11 bits to represent; since block size is minimum 1, we can use 0000000000 to represent it
    *(base_address + 3) = *(base_address + 3) >> 8; //6 bits for size, 1 bit for unallocated, 1 bit don't care
    printf("Base Address: %d %x  %d\n", base_address, base_address, &base_address);

    //initializing elements of bin_ptrs array to NULL
    for(i = 0; i < 11; ++i)
    {
        bin_ptrs[i] = NULL;
        printf("Bin Ptrs %p", bin_ptrs[i]);
    }
    bin_ptrs[10] = base_address; //point the initial bin to block size 32768 (32*1024)

    half_init_unallocated_block (base_address);

}

void half_alloc_helper(char *block_addr, U32 bin_index){
    U32 next_block_in_current_bin;
    char *nextBlockInCurrentBin;
    char *temp;

    //update the bin so that it it now points to the next block in that bin
    next_block_in_current_bin = (((U32)(*(block_addr + 5) & 00111111)) << 4) + (((U32)(*(block_addr + 6) & 11110000)) >> 6); //between 0 and 1023
    nextBlockInCurrentBin = ((char *) next_block_in_current_bin);

    if(nextBlockInCurrentBin == block_addr){
        bin_ptrs[bin_index] = NULL;
    } else {
        bin_ptrs[bin_index] = (next_block_in_current_bin << 5) + base_address;

        //For the next block available in the bin, we must update the previous ptr to point to itself
        //the previous ptr is represented by bits 32-42
        *(nextBlockInCurrentBin + 4) = (char *)(next_block_in_current_bin >> 2);
        *(nextBlockInCurrentBin + 5) = (char *)((((next_block_in_current_bin & 0000000011)) << 6) + ((U32)(*(nextBlockInCurrentBin + 5) & 00111111))) ;
    }

    //update the bit in the block so that it is indicates allocated without affecting the remaining bits in that 4th header byte
    *(block_addr + 3) = (*(block_addr + 3) & 11111100) | 00000010;


}

char *half_alloc( U32 n) {
    U32 bin_index;
    char *mem_block;
    U32 mem_block_size; //size of the available memory block

    n += 4; //including the size of the mandatory 4 byte header

    //Determining the index of the bin that we want to start looking in
    if(n < 32)
    {
        bin_index = 0;
    }
    else
    {
        bin_index = ceil(log(n)/log(2)) - 5;

        //if the index is greater than 10, we cannot allocate that memory so we return null
        if (bin_index > 10)
        {
            return NULL;
        }
    }
    //at this point, bin_index will definitely have a value

    //now that we know what bin to start looking into, we should look into that bin for an available memory block
    //if the bin is empty, we should look in the bin of the next size greater than that
    //we should continue to do this until all bins have been traversed
    //checking to see if the current bin to look in is empty
    while (bin_ptrs[bin_index] == NULL && bin_index < 11){
        bin_index++;
    }
    //we have now found the bin that we can retrieve a block of memory to allocate from or we have bin_index = 11 in which case we cannot allocate any memory
    if(bin_index == 11){
        return NULL;
    }
    //at this point, we have established that there is a bin with an available block of memory that we can allocate

    /*
     if available_mem_block_size - n < 32, allocate the entire block
     if available_mem_block_size - n >= 32, split the block into a two blocks:
        - one with size = smallest multiple of 32 that is greater than n
        - second with size = size of the entire block - size of the first block

        - then we allocate the 1st block
        - place the second block into a bin that it corresponds to

        --> When we allocate a block of memory, we must:
            - update the bin so that it it now points to the next block in that bin
            - we DO NOT need to touch the next ptr and prev ptr in memory because they do not change
            - update the previous ptr of the next block in the memory to point to itself
            - update the bit in the header that indicates that the block is allocated
            - allocate the first available block in that bin
        --> When we split a block of memory into two blocks as mentioned above, we must:
            - update the next and previous memory ptrs each block before or after the two new blocks
            - update the bin that the original block (the block before we split it) came from so that it now points to the
              next block in that bin
            - initialize the header for each of the two new blocks
            - allocate the first block that we split
            - deallocate the second block (NOTE: the block was never allocated but in order to reassign it into an appropriate
              bin, we can treat it as though it is being deallocated
    */

    //let's start by getting the pointer to the block of memory we want to allocate
    mem_block = bin_ptrs[bin_index];

    // Now to perform the appropriate memory allocations
    //Retrieving the size of the memory block we will be allocating/splitting
    mem_block_size = (((U32)((*(mem_block + 2)) & 00001111)) << 6) + (((U32)((*(mem_block + 3)) & 11111100)) >> 4);
    printf("SIZE: %d \n", mem_block_size);
    if(mem_block_size == 0){
        mem_block_size = 1024;
    }
    printf("SIZE: %d", mem_block_size);

    if((mem_block_size*32 - n) < 32) {
        half_alloc_helper (mem_block, bin_index);
        return mem_block;
    } else {
        //split the block into two and then allocate it
    }

}

/*
    When the request comes in to free a block, we need to update several things:
    There are two cases:
    1. Block that is being deallocated is stand-alone (meaning there are only allocated blocks next to it)
       -> If this is the case, we use the same steps as the other case.
       -> find out the appropriate bucket that this deallocated block should go in
    2. Block that is being deallocated has free (unallocated) blocks next to it.
       - in this case, we need to combine the adjacent unallocated blocks into one larger free block
       -> First check which one is free (either left or right of the current block being deallocated)
       -> Then we need to combine the headers into one header containing the new information.
          -> If the new combined block is at the end of the 1024 size initial block, the pointer to the next block
             in memory will be assigned to itself (?? or zero?)
          -> If the new combined block is in the middle of the initialized block (between other allocated blocks),
             Then depending on whether the other block that is free (next to the one being deallocated) is to the
             left or right of the block being deallocated, we have to update the pointer to the previous or next
             memory block
          -> Update the size of the new, combined block (remember to add the previous size of the block that we joined
             to the one being deallocated.
*/

void  half_free( char * mem_block);
