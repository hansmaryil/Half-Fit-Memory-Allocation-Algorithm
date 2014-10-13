#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include "half_fit.h"
#include "type.h"

//global variable declaration
half_fit_t half_fit;
U32 *base_address;
U32 init_size = 8192; //each U32 int is 4 bytes; since we need 32768 bytes in total, 8192 U32 ints will be its equivalent
U32 *bin_ptrs[11]; //initialize an array of uint pointers to point to the first block in each bin (initially NULL)
void half_free(U32 *free_block);

void  half_init( void )
{
    U32 i;
    base_address = (U32 *) malloc(init_size);

    //initializing header (first 4 bytes; A.K.A the first U32 int)
    *base_address = 0;
    //(*(base_address)) = ((*base_address) << 32); //try to find out why this line doesnot work properly

    //we are only interested in making the first two bytes of the 2nd U32 int to "0" but it does not matter what we do to the remainder of the integer
    *(base_address + 1) = 0;

    //initializing elements of bin_ptrs array to NULL
    for(i = 0; i < 11; ++i)
    {
        bin_ptrs[i] = NULL;
    }
    bin_ptrs[10] = base_address; //our initial block belongs in bin 10
}

//function to retrieve the size from the header of a block of memory
U32 getSize( U32 *mem_block){
    U32 block_size;
    /*- To get the size, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 31-40
      - To do this, we bitmask using AND (&) with 0000-0000-0000-0000-0000-1111-1111-1100 = 4092 in decimal
      - Then, we bit-shift to the right by 2
    */

    block_size = ((*(mem_block)) & 4092) >> 2;
    //we are storing a size of 1024 as "0" because 1024 cannot be represented in 10-bits and
    //since we know that the minimum block size is 1, we can use 0 to represent 1024 without causing any issues
    if(block_size == 0){
        block_size = 1024;
    }
    return block_size;
}

//function to set the size in the header of a block of memory
void setSize( U32 *mem_block, U32 size){
    /*we must take size and make have it represented through 10 bits
      - function assumes that size is already between 0 and 1023 (meaning it is the number of 32 byte memory blocks in the entire block)
      - we are interested in making bits 21-30 in the U32 header to the value of size
      - we shift the size by 2 bits to the left
      - we then bitmask AND (&) the entire header with 1111-1111-1111-1111-1111-0000-0000-0011 = 4294963203 in decimal
        - this ensures that all values in the header are kept the same and size is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-0000-0000-0000-(s1)(s2)(s3)(s4)-(s5)(s6)(s7)(s8)-(s9)(s10)00
        where (s1) to (s10) represent the 10 individual bits in the size representation
    */
    U32 header;

    size = size << 2;
    header = *mem_block & 4294963203;

    header = header | size;
    *mem_block = header;
}

//function to retrieve the pointer to the previous block in memory from the header of the block passed in as a parameter
U32 *getPrevInMemory( U32 *mem_block){
    U32 prevInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the previous block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
    */

    prevInMemoryReferenceAddress = ((*(mem_block)) & 4290772992) >> 22;

    //To get the actual previous address out of this, we must multiply prevInMemoryReferenceAddress by 32 and add our base address to it
    if((base_address + prevInMemoryReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + prevInMemoryReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in memory in the header of the block passed in as a parameter
void setPrevInMemory(U32 *mem_block, U32 *prevInMemory){
    /*we must take prevInMemory and make have it represented through 10 bits
      - we are interested in making bits 1-10 in the U32 header to the value of prevInMemory
      - we convert prevInMemory to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 22
      - we then bitmask AND (&) the entire header with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInMemory is turned to 0
      - we then bitmask OR (|) the entire header with (p1)(p2)(p3)(p4)-(p5)(p6)(p7)(p8)-(p9)(p10)00-0000-0000-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the prevInMemory representation
    */
    U32 header;
    U32 prevInMemoryRefAdd;
    U32 prevInMemoryVal;
    U32 base_address_val;

    prevInMemoryVal = (U32) prevInMemory;
    base_address_val = (U32) base_address;

    prevInMemoryRefAdd = ((prevInMemoryVal - base_address_val)/32);
    prevInMemoryRefAdd = prevInMemoryRefAdd << 22;

    header = *mem_block;
    header = header & 4194303;

    header = header | prevInMemoryRefAdd;
    *mem_block = header;
}

//function to retrieve the pointer to the next block in memory from the header of the block passed in as a parameter
U32 *getNextInMemory( U32 *mem_block){
    U32 nextInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the next block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
    */

    nextInMemoryReferenceAddress = ((*(mem_block)) & 4190208) >> 12;

    //To get the actual next address out of this, we must multiply prevInMemoryReferenceAddress by 32 and add our base address to it
    if((base_address + nextInMemoryReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + nextInMemoryReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in memory in the header of the block passed in as a parameter
void setNextInMemory(U32 *mem_block, U32 *nextInMemory){
    /*we must take nextInMemory and make have it represented through 10 bits
      - we are interested in making bits 11-20 in the U32 header to the value of prevInMemory
      - we convert prevInMemory to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 12
      - we then bitmask AND (&) the entire header with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the header are kept the same and nextInMemory is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInMemory representation
    */
    U32 header;
    U32 nextInMemoryRefAdd;
    U32 nextInMemoryVal;
    U32 base_address_val;

    nextInMemoryVal = (U32) nextInMemory;
    base_address_val = (U32) base_address;

    nextInMemoryRefAdd = ((nextInMemoryVal - base_address_val)/32);
    nextInMemoryRefAdd = nextInMemoryRefAdd << 12;

    header = *mem_block;
    header = header & 4290777087;

    header = header | nextInMemoryRefAdd;
    *mem_block = header;
}

//function to retrieve the reference address (between 0 and 1023) to the previous block in memory from the header of the block passed in as a parameter
//NOTE: Unlike the getPrevInMemory function, this function DOES NOT/CANNOT return NULL if the previous pointer is pointing to itself (mem_block)
U32 getPrevInMemoryReferenceAddress( U32 *mem_block){
    U32 prevInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the previous block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
    */
    prevInMemoryReferenceAddress = ((*(mem_block)) & 4290772992) >> 22;

    return prevInMemoryReferenceAddress;
}

//function to set the reference address to the previous block in memory in the header of the block passed in as a parameter
void setPrevInMemoryReferenceAddress(U32 *mem_block, U32 prevInMemoryRefAddress){
    /*we must take prevInMemory and make have it represented through 10 bits
      - we bitmask AND (&) the entire header with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInMemory is turned to 0
      - we then bitmask OR (|) the entire header with prevInMemoryRefAddress shifted left by 22
    */
    U32 header;

    header = *mem_block;
    header = header & 4194303;

    header = header | (prevInMemoryRefAddress << 22);
    *mem_block = header;
}

//function to retrieve the reference address (between 0 and 1023) to the next block in memory from the header of the block passed in as a parameter
//NOTE: Unlike the getNextInMemory function, this function DOES NOT/CANNOT return NULL if the next pointer is pointing to itself (mem_block)
U32 getNextInMemoryReferenceAddress( U32 *mem_block){
    U32 nextInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the next block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
    */

    nextInMemoryReferenceAddress = ((*(mem_block)) & 4190208) >> 12;
    return nextInMemoryReferenceAddress;
}

//function to set the reference address to the next block in memory in the header of the block passed in as a parameter
void setNextInMemoryReferenceAddress(U32 *mem_block, U32 nextInMemoryRefAddress){
    /*we must take nextInMemory and make have it represented through 10 bits
      - we bitmask AND (&) the entire header with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the header are kept the same and nextInMemory is turned to 0
      - we then bitmask OR (|) the entire header with nextInMemoryRefAddress shifted left by 12
    */
    U32 header;

    header = *mem_block;
    header = header & 4290777087;

    header = header | (nextInMemoryRefAddress << 12);
    *mem_block = header;
}

//function to determine whether or not a memory block is allocated
inline bool isAllocated(U32 *mem_block){
    /*To determine if a memory block is allocated, we are interested in determining what is in the 31 bit in the header
      - To get the 31st bit, we bitmask using AND (&) with 0000-0000-0000-0000-0000-0000-0000-0010 = 2 in decimal
      - Then, we bit-shift to the right by 1
    */
    return ((((*(mem_block)) & 2) >> 1) == 1) ? 1 : 0;
}

//function activates the boolean variable that indicates a block is allocated
void setAllocate(U32 *mem_block){
    /* To allocate the boolean variable in the header to 1 (bit 31), we must:
       - we bitmask OR (|) the header with 0000-0000-0000-0000-0000-0000-0000-0010 = 2 in decimal
    */
    *(mem_block) = (*(mem_block)) | 2;
}

//function de-activates the boolean variable that indicates a block is allocated
void setDeallocate(U32 *mem_block){
    /* To allocate the boolean variable in the header to 1 (bit 31), we must:
       - we bitmask AND (&) the header with 1111-1111-1111-1111-1111-1111-1111-1101 = 4294967293 in decimal
    */
    *(mem_block) = (*(mem_block)) & 4294967293;
}

//function to retrieve the pointer to the previous block in a bin from the 2nd U32 integer of the block passed in as a parameter
U32 *getPrevInBin( U32 *mem_block){
    U32 prevInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the prev block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
    */

    prevInBinReferenceAddress = ((*(mem_block + 1)) & 4290772992) >> 22;

    //To get the actual prev address out of this, we must multiply prevInBinReferenceAddress by 32 and add our base address to it
    if((base_address + prevInBinReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + prevInBinReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in bin in the header of the block passed in as a parameter
void setPrevInBin(U32 *mem_block, U32 *prevInBin){
    /*we must take prevInBin and make have it represented through 10 bits
      - we are interested in making bits 1-10 in the second u32 integer to the value of prevInBin
      - we convert prevInBin to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 22
      - we then bitmask AND (&) the entire second u32 integer with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInBin is turned to 0
      - we then bitmask OR (|) the entire header with (p1)(p2)(p3)(p4)-(p5)(p6)(p7)(p8)-(p9)(p10)00-0000-0000-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the prevInBin representation
    */
    U32 header_part2;
    U32 prevInBinRefAdd;
    U32 prevInBinVal;
    U32 base_address_val;

    prevInBinVal = (U32) prevInBin;
    base_address_val = (U32) base_address;

    prevInBinRefAdd = ((prevInBinVal - base_address_val)/32);
    prevInBinRefAdd = prevInBinRefAdd << 22;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4194303;

    header_part2 = header_part2 | prevInBinRefAdd;
    *(mem_block + 1) = header_part2;
}

//function to retrieve the pointer to the next block in a bin from the 2nd U32 integer of the block passed in as a parameter
U32 *getNextInBin( U32 *mem_block){
    U32 nextInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the next block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
    */

    nextInBinReferenceAddress = ((*(mem_block + 1)) & 4190208) >> 12;

    //To get the actual next address out of this, we must multiply prevInBinReferenceAddress by 32 and add our base address to it
    if((base_address + nextInBinReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + nextInBinReferenceAddress*(32/4));
}

//function to set the pointer to the next block in bin in the header of the block passed in as a parameter
void setNextInBin(U32 *mem_block, U32 *nextInBin){
    /*we must take nextInBin and make have it represented through 10 bits
      - we are interested in making bits 11-20 in the second u32 integer to the value of nextInBin
      - we convert nextInBin to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 12
      - we then bitmask AND (&) the entire second u32 integer with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the second U32 integer are kept the same and nextInBin is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInBin representation
    */
    U32 header_part2;
    U32 nextInBinRefAdd;
    U32 nextInBinVal;
    U32 base_address_val;

    nextInBinVal = (U32) nextInBin;
    base_address_val = (U32) base_address;

    nextInBinRefAdd = ((nextInBinVal - base_address_val)/32);
    nextInBinRefAdd = nextInBinRefAdd << 12;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4290777087;

    header_part2 = header_part2 | nextInBinRefAdd;
    *(mem_block + 1) = header_part2;
}

//function to retrieve the reference address (between 0 and 1023) to the previous block in a bin from the 2nd U32 integer of the block passed in as a parameter
//NOTE: Unlike the getPrevInBin function, this function DOES NOT/CANNOT return NULL if the next pointer is pointing to itself (mem_block)
U32 getPrevInBinReferenceAddress( U32 *mem_block){
    U32 prevInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the prev block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
    */

    return (((*(mem_block + 1)) & 4290772992) >> 22);
}

//function to set the reference address to the previous block in bin in the 2nd U32 integer of the block passed in as a parameter
void setPrevInBinReferenceAddress(U32 *mem_block, U32 prevInBinRefAddress){
    /*we must take prevInBin and have it represented through 10 bits
      - we bitmask AND (&) the entire header_part2 with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header_part2 are kept the same and prevInBin is turned to 0
      - we then bitmask OR (|) the entire header with prevInBinRefAddress shifted left by 22
    */
    U32 header_part2;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4194303;

    header_part2 = header_part2 | (prevInBinRefAddress << 22);
    *(mem_block + 1) = header_part2;
}

//function to retrieve the reference address (between 0 and 1023) to the next block in a bin from the 2nd U32 integer of the block passed in as a parameter
//NOTE: Unlike the getNextInBin function, this function DOES NOT/CANNOT return NULL if the next pointer is pointing to itself (mem_block)
U32 getNextInBinReferenceAddress( U32 *mem_block){
    U32 nextInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to
    /*- To get the pointer to the next block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
    */

    return (((*(mem_block + 1)) & 4190208) >> 12);
}

//function to set the reference address to the next block in bin in the 2nd U32 integer of the block passed in as a parameter
void setNextInBinReferenceAddress(U32 *mem_block, U32 nextInBinRefAddress){
    /*we must take nextInBin and have it represented through 10 bits
      - we bitmask AND (&) the entire second u32 integer with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the second U32 integer are kept the same and nextInBin is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInBin representation
    */
    U32 header_part2;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4290777087;

    header_part2 = header_part2 | (nextInBinRefAddress << 12);
    *(mem_block + 1) = header_part2;
}

void half_alloc_helper(U32 *block_addr, U32 bin_index){
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

U32 *half_alloc( U32 n) {
    U32 bin_index;
    U32 *mem_block;
    U32 mem_block_size; //size of the available memory block

    n += 4; //including the size of the mandatory 4 byte header; also, we add 4 as n represents the number of bytes

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
    mem_block_size = getSize(mem_block);

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

//this helper fcn inserts a block in the correct bin at the front of all blocks in the bin
void insert_block_in_bin( U32 *block_being_inserted, U32 bin_index )
{
    if( bin_ptrs[bin_index] == NULL) //if there are no blocks in the bin, we can simply point the bin in the correct index to the new block being inserted into the bin
    {
        bin_ptrs[bin_index] = block_being_inserted;
        setPrevInBin(block_being_inserted, block_being_inserted); // since this is the only block in the bin, we will set the pointer to the previous block to point to itself
        setNextInBin(block_being_inserted, block_being_inserted); // since this is the only block in the bin, we will set the pointer to the next block to point to itself
    }

    setPrevInBin(bin_ptrs[bin_index], block_being_inserted); //set the original first block's previous pointer in the bin to point to the new block we are inserting
    setNextInBin(block_being_inserted, bin_ptrs[bin_index]); //set the new block's (the one we are inserting) next pointer in the bin to point to the previous first block in the bin
    bin_ptrs[bin_index] = block_being_inserted; // update the bin_ptr in the appropriate index to now point to the new block at the front of the bin
}

//this helper function will update the appropriate bin when passing in the pointers to the address of the previous and next blocks in bin as well as the adjacent block in main memory that needs to be updated
void update_bin_when_reallocating( U32 *adjacent_memory_block )
{
    U32 address_of_prev_block_in_bin;
    U32 address_of_next_block_in_bin;

    address_of_prev_block_in_bin = getPrevInBin(adjacent_memory_block); //gets the address of the prev block in the bin the adjacent unallocated block is in
    address_of_next_block_in_bin = getNextInBin(adjacent_memory_block); //gets the address of the next block in the bin the adjacent unallocated block is in

    if( (address_of_prev_block_in_bin == NULL) && (address_of_next_block_in_bin == NULL) ) // checks if there are no other members in the bin
    {
        bin_ptrs[(U32) floor(log(getSize(adjacent_memory_block) / log(2)))] = NULL; //sets the bin ptr of the appropriate block size to NULL (meaning the bin is empty)
    }
    else if ( (address_of_prev_block_in_bin == NULL) && (address_of_next_block_in_bin != NULL) ) // checks if this is the first block in the bin
    {
        bin_ptrs[(U32) floor(log(getSize(adjacent_memory_block) / log(2)))] = address_of_next_block_in_bin; //the bin pointer for the size of adjacent unallocated block now points to the next block in the adjacent block's bucket
    }
    else if ( (address_of_prev_block_in_bin != NULL) && (address_of_next_block_in_bin != NULL) ) // checks if this in a block in the bin between two other blocks in the same bin
    {
        setNextInBin(address_of_prev_block_in_bin, address_of_next_block_in_bin); // set the next pointer for the block to the left of adjacent_memory_block to point to the block to the right of adjacent_memory_block
        setPrevInBin(address_of_prev_block_in_bin, address_of_prev_block_in_bin); // set the previous pointer for the block to the right of adjacent_memory_block to point to the block to the left of adjacent_memory_block
    }
    else  // the last case is if the adjacent, unallocated block is the last block in its respective bin
    {
        setNextInBin(address_of_prev_block_in_bin, address_of_prev_block_in_bin); //set the pointer in the bin of the block to the left of the unallocated block to itself (indicating it is the last block in that bin)
    }
}

void  half_free( U32 *free_block )
{
    U32 *address_of_next_block_in_memory;
    U32 *address_of_prev_block_in_memory;
    U32 *address_of_prev_block_in_bin;
    U32 *address_of_next_block_in_bin;
    U32 new_size;
    U32 new_bin_index;

    address_of_next_block_in_memory = getNextInMemory(free_block); //use helper fcn to get the next address in memory
    address_of_prev_block_in_memory = getPrevInMemory(free_block); //use helper fcn to get the previous address in memory


    // ***** 1. FIRST CASE WHERE THE BLOCK TO BE FREED IS STANDALONE (no free blocks next to it) ***** \\

    if( ((address_of_prev_block_in_memory == NULL) && (!(isAllocated(address_of_next_block_in_memory)))) || ((address_of_next_block_in_memory == NULL) && (!(isAllocated(address_of_prev_block_in_memory))))
        || ((!(isAllocated(address_of_prev_block_in_memory))) && (!(isAllocated(address_of_next_block_in_memory)))) )
    {
        new_bin_index = floor(log(getSize(free_block)) / log(2)); //the appropriate bin that the block to be freed must be put into

        // if there are no adjacent free blocks, we simply need to update the bin that the freed block must go into
        insert_block_in_bin(address_of_prev_block_in_memory, new_bin_index); // insert the block in the front of the appropriate bin
        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored the block
    }

    // ***** 2. WE'LL TAKE THE CASE IF THE BLOCK BEING DEALLOCATED HAS ONLY A FREE BLOCK TO THE RIGHT OF IT ***** \\

    else if( (address_of_prev_block_in_memory == NULL) || (isAllocated(address_of_prev_block_in_memory) && !(isAllocated(address_of_next_block_in_memory))) ) // check that only the block to the right of free_block is free
    {
        U32 *address_of_next_allocated_block;

        new_size = getSize(address_of_next_block_in_memory) + getSize(free_block); //gets the size of   the new, combined block
        new_bin_index = floor(log(new_size) / log(2)); //get the new bin index for the combined size of the free blocks


        // we need to update the values in the header of both the free_block and the next allocated block of memory in sequence
        if( getNextInMemory(address_of_next_block_in_memory) != NULL ) // this checks if the free block to the right of the block to be freed is the only other free block in the whole of memory
        {
            address_of_next_allocated_block = getNextInMemory(address_of_next_block_in_memory); // get the address of the next allocated block in the whole memory (after the block that is to be freed)
            setNextInMemory(free_block, address_of_next_allocated_block); // make the pointer (in free_block) that points to the next block in memory point to the next allocated block in memory
            setPrevInMemory(address_of_next_allocated_block, free_block); // make the pointer (in the next allocated block) that points to the previous block in memory point to free_block
        }
        else // at this point, we know that the free block to our right is the only other block in memory
        {
            setNextInMemory(free_block, free_block); // make the pointer (in free_block) that points to the next block in memory point to itself (because it is now the last block in memory
        }

        // check to see in which bin the new, combined block has to go into
        if( new_bin_index >= ((floor(log(getSize(free_block)) / log(2))) + 1)  )
        {
            insert_block_in_bin(free_block, new_bin_index); // insert the block in the front of the appropriate bin
            setSize(free_block, new_size);  // update the size of the new combined block
        }

        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored the block
    }


    // ***** 3. TAKE THE CASE IF THE BLOCK BEING DEALLOCATED HAS TWO ADJACENT FREE BLOCKS NEXT TO IT ***** \\

    //check that there are two free adjacent blocks and that we are not at the end of the whole memory block
    else if( (!(isAllocated(address_of_prev_block_in_memory))) && (!(isAllocated(address_of_next_block_in_memory))) && (address_of_next_block_in_memory != NULL) )
    {
        U32 *new_head_of_combined_free_block;
        U32 *address_of_next_allocated_block;

        new_size = getSize(address_of_prev_block_in_memory) + getSize(free_block) + getSize(address_of_next_block_in_memory); //gets the size of the new, combined block
        new_bin_index = (U32) floor(log(new_size) / log(2)); //get the new bin index for the combined size of the free blocks

        update_bin_when_reallocating( address_of_next_block_in_memory ); //update the bin that the adjacent unallocated block to the right of free_block is in

        // we need to update the values in the header of both the free_block and the next allocated block of memory in sequence (if necessary)
        if( getNextInMemory(address_of_next_block_in_memory) != NULL ) //this checks to ensure that the free block to the right is not the last block in memory
        {
            address_of_next_allocated_block = getNextInMemory(address_of_next_block_in_memory); // get the address of the next allocated block in the whole memory (after the block that is to be freed)
            setNextInMemory(address_of_prev_block_in_memory, address_of_next_allocated_block); // make the pointer (in address_of_prev_block_in_memory) that points to the next block in memory point to the next allocated block in memory
            setPrevInMemory(address_of_next_allocated_block, address_of_prev_block_in_memory); // make the pointer (in the next allocated block) that points to the previous block in memory point to address_of_prev_block_in_memory (which is the one to the left of free_block)
        }
        else
        {
            setNextInMemory(address_of_prev_block_in_memory, address_of_prev_block_in_memory); // make the pointer (in address_of_prev_block_in_memory) that points to the next block in memory point to itself (indicating that this is the last block in memory with no more blocks to the right of it)
        }

        // check to see in which bin the new, combined block has to go into
        if( new_bin_index >= ((U32) ((floor(log(getSize(free_block)) / log(2))) + 1)) )
        {
            update_bin_when_reallocating( address_of_prev_block_in_memory ); //update the bin that the adjacent unallocated block to the left of free_block is in - only update this block in its bin if the new size, combined size is greater than what should be in its current bin
            insert_block_in_bin(address_of_prev_block_in_memory, new_bin_index); // insert the block in the front of the appropriate bin
            setSize(address_of_prev_block_in_memory, new_size);  // update the size of the new combined block
        }

        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored in the block
    }


    // ***** 4. WE'LL TAKE THE CASE IF THE BLOCK BEING DEALLOCATED HAS ONLY A FREE BLOCK TO THE LEFT OF IT ***** \\

    else //if( ((address_of_next_block_in_memory == NULL) && (!(inline isAllocated(address_of_prev_block_in_memory)))) || (!(inline isAllocated(address_of_prev_block_in_memory)) && (inline isAllocated(address_of_next_block_in_memory))) ) // check that only the block to the right of free_block is free)
    {
        U32 *address_of_next_allocated_block;

        new_size = getSize(address_of_prev_block_in_memory) + getSize(free_block); //gets the size of the new, combined block
        new_bin_index = (U32) floor(log(new_size) / log(2)); //get the new bin index for the combined size of the free blocks

        // we need to update the values in the header of the previous unallocated block of memory in sequence to point to the next allocated block in sequence
        if( (address_of_next_block_in_memory == NULL) ) // if this is the last block in the whole of memory (no more blocks to its right), we need to update the free block to the left of the block to be freed to point to itself (indicating that its the last block in the whole of memory)
        {
            setNextInMemory(address_of_prev_block_in_memory, address_of_prev_block_in_memory); // make the pointer (in address_of_prev_block_in_memory) that points to the next block in memory point to itself
        }
        else // if the above condition is not met, then we know that there is an allocated block to the right of the block we are going to free
        {
            address_of_next_allocated_block = getNextInMemory(free_block); // get the address of the next allocated block in the whole memory (after the block that is to be freed)
            setNextInMemory(address_of_prev_block_in_memory, address_of_next_allocated_block); // make the pointer (in address_of_prev_block_in_memory) that points to the next block in memory point to the next allocated block in memory
            setPrevInMemory(address_of_next_allocated_block, address_of_prev_block_in_memory); // make the pointer (in the next allocated block) that points to the previous block in memory point to address_of_prev_block_in_memory (which is the one to the left of free_block)
        }

        // check to see in which bin the new, combined block has to go into
        if( new_bin_index >= ((U32) ((floor(log(getSize(address_of_prev_block_in_memory)) / log(2))) + 1)) )
        {
            update_bin_when_reallocating( address_of_prev_block_in_memory ); //update the bin that the adjacent unallocated block to the left of free_block is in - only update this block in its bin if the new size, combined size is greater than what should be in its current bin
            insert_block_in_bin(address_of_prev_block_in_memory, new_bin_index); // insert the block in the front of the appropriate bin
            setSize(address_of_prev_block_in_memory, new_size);  // update the size of the new combined block
        }

        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored in the block
    }


}

