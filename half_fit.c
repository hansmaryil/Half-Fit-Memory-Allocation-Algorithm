#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include "half_fit.h"
#include "type.h"

//global variable declaration
half_fit_t half_fit;
U32 *base_address;
U32 init_size = 8191; //each U32 int is 4 bytes; since we need 32768 bytes in total, 8192 U32 ints will be its equivalent
U32 *bin_ptrs[11]; //initialize an array of uint pointers to point to the first block in each bin (initially NULL)

void  half_init( void ){
    U32 i;
    base_address = (U32 *) malloc(init_size);

    //initializing header (first 4 bytes; A.K.A the first U32 int)
    *base_address = 0;
    //we are only interested in making the first two bytes of the 2nd U32 int to "0" but it does not matter what we do to the remainder of the integer
    *(base_address + 1) = 0;

    //initializing elements of bin_ptrs array to NULL
    for(i = 0; i < 10; ++i)
    {
        bin_ptrs[i] = NULL;
    }
    bin_ptrs[10] = base_address; //our initial block belongs in bin 10
}

//function to retrieve the size from the header of a block of memory
/*- To get the size, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 31-40
      - To do this, we bitmask using AND (&) with 0000-0000-0000-0000-0000-1111-1111-1100 = 4092 in decimal
      - Then, we bit-shift to the right by 2
*/
U32 getSize( U32 *mem_block){
    U32 block_size;

    block_size = ((*(mem_block)) & 4092) >> 2;
    //we are storing a size of 1024 as "0" because 1024 cannot be represented in 10-bits and
    //since we know that the minimum block size is 1, we can use 0 to represent 1024 without causing any issues
    if(block_size == 0){
        block_size = 1024;
    }
    return block_size;
}

//function to set the size in the header of a block of memory
/*we must take size and make have it represented through 10 bits
      - function ASSUMES that size is already between 0 and 1023 (meaning it is the number of 32 byte memory blocks in the entire block)
      - we are interested in making bits 21-30 in the U32 header to the value of size
      - we shift the size by 2 bits to the left
      - we then bitmask AND (&) the entire header with 1111-1111-1111-1111-1111-0000-0000-0011 = 4294963203 in decimal
        - this ensures that all values in the header are kept the same and size is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-0000-0000-0000-(s1)(s2)(s3)(s4)-(s5)(s6)(s7)(s8)-(s9)(s10)00
        where (s1) to (s10) represent the 10 individual bits in the size representation
*/
void setSize( U32 *mem_block, U32 size){
    U32 header;

    size = size << 2;
    header = *mem_block & 4294963203;

    header = header | size;
    *mem_block = header;
}

//function to retrieve the pointer to the previous block in memory from the header of the block passed in as a parameter
/*- To get the pointer to the previous block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
*/
U32 *getPrevInMemory( U32 *mem_block){
    U32 prevInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    prevInMemoryReferenceAddress = ((*(mem_block)) & 4290772992) >> 22;

    //To get the actual previous address out of this, we must multiply prevInMemoryReferenceAddress by 32 and add our base address to it
    if((base_address + prevInMemoryReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + prevInMemoryReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in memory in the header of the block passed in as a parameter
/*we must take prevInMemory and make have it represented through 10 bits
      - we are interested in making bits 1-10 in the U32 header to the value of prevInMemory
      - we convert prevInMemory to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 22
      - we then bitmask AND (&) the entire header with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInMemory is turned to 0
      - we then bitmask OR (|) the entire header with (p1)(p2)(p3)(p4)-(p5)(p6)(p7)(p8)-(p9)(p10)00-0000-0000-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the prevInMemory representation
*/
void setPrevInMemory(U32 *mem_block, U32 *prevInMemory){
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
/*- To get the pointer to the next block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
*/
U32 *getNextInMemory( U32 *mem_block){
    U32 nextInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    nextInMemoryReferenceAddress = ((*(mem_block)) & 4190208) >> 12;

    //To get the actual next address out of this, we must multiply prevInMemoryReferenceAddress by 32 and add our base address to it
    if((base_address + nextInMemoryReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + nextInMemoryReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in memory in the header of the block passed in as a parameter
/*we must take nextInMemory and make have it represented through 10 bits
      - we are interested in making bits 11-20 in the U32 header to the value of prevInMemory
      - we convert prevInMemory to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 12
      - we then bitmask AND (&) the entire header with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the header are kept the same and nextInMemory is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInMemory representation
*/
void setNextInMemory(U32 *mem_block, U32 *nextInMemory){
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
/*- To get the pointer to the previous block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
*/
U32 getPrevInMemoryReferenceAddress( U32 *mem_block){
    U32 prevInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    prevInMemoryReferenceAddress = ((*(mem_block)) & 4290772992) >> 22;
    return prevInMemoryReferenceAddress;
}

//function to set the reference address to the previous block in memory in the header of the block passed in as a parameter
/*we must take prevInMemory and make have it represented through 10 bits
      - we bitmask AND (&) the entire header with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInMemory is turned to 0
      - we then bitmask OR (|) the entire header with prevInMemoryRefAddress shifted left by 22
*/
void setPrevInMemoryReferenceAddress(U32 *mem_block, U32 prevInMemoryRefAddress){
    U32 header;

    header = *mem_block;
    header = header & 4194303;

    header = header | (prevInMemoryRefAddress << 22);
    *mem_block = header;
}

//function to retrieve the reference address (between 0 and 1023) to the next block in memory from the header of the block passed in as a parameter
//NOTE: Unlike the getNextInMemory function, this function DOES NOT/CANNOT return NULL if the next pointer is pointing to itself (mem_block)
/*- To get the pointer to the next block in memory, we are interested in the first U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
*/
U32 getNextInMemoryReferenceAddress( U32 *mem_block){
    U32 nextInMemoryReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    nextInMemoryReferenceAddress = ((*(mem_block)) & 4190208) >> 12;
    return nextInMemoryReferenceAddress;
}

//function to set the reference address to the next block in memory in the header of the block passed in as a parameter
/*we must take nextInMemory and make have it represented through 10 bits
      - we bitmask AND (&) the entire header with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the header are kept the same and nextInMemory is turned to 0
      - we then bitmask OR (|) the entire header with nextInMemoryRefAddress shifted left by 12
*/
void setNextInMemoryReferenceAddress(U32 *mem_block, U32 nextInMemoryRefAddress){
    U32 header;

    header = *mem_block;
    header = header & 4290777087;

    header = header | (nextInMemoryRefAddress << 12);
    *mem_block = header;
}

//function to determine whether or not a memory block is allocated
/*To determine if a memory block is allocated, we are interested in determining what is in the 31 bit in the header
      - To get the 31st bit, we bitmask using AND (&) with 0000-0000-0000-0000-0000-0000-0000-0010 = 2 in decimal
      - Then, we bit-shift to the right by 1
*/
inline bool isAllocated(U32 *mem_block){
    return ((((*(mem_block)) & 2) >> 1) == 1) ? 1 : 0;
}

//function activates the boolean variable that indicates a block is allocated
/* To allocate the boolean variable in the header to 1 (bit 31), we must:
       - we bitmask OR (|) the header with 0000-0000-0000-0000-0000-0000-0000-0010 = 2 in decimal
*/
void setAllocate(U32 *mem_block){
    *(mem_block) = (*(mem_block)) | 2;
}

//function de-activates the boolean variable that indicates a block is allocated
/* To allocate the boolean variable in the header to 1 (bit 31), we must:
       - we bitmask AND (&) the header with 1111-1111-1111-1111-1111-1111-1111-1101 = 4294967293 in decimal
*/
void setDeallocate(U32 *mem_block){
    *(mem_block) = (*(mem_block)) & 4294967293;
}

//function to retrieve the pointer to the previous block in a bin from the 2nd U32 integer of the block passed in as a parameter
/*- To get the pointer to the prev block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
*/
U32 *getPrevInBin( U32 *mem_block){
    U32 prevInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    prevInBinReferenceAddress = ((*(mem_block + 1)) & 4290772992) >> 22;
    //To get the actual prev address out of this, we must multiply prevInBinReferenceAddress by 32 and add our base address to it
    if((base_address + prevInBinReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + prevInBinReferenceAddress*(32/4));
}

//function to set the pointer to the previous block in bin in the header of the block passed in as a parameter
/*we must take prevInBin and make have it represented through 10 bits
      - we are interested in making bits 1-10 in the second u32 integer to the value of prevInBin
      - we convert prevInBin to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 22
      - we then bitmask AND (&) the entire second u32 integer with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header are kept the same and prevInBin is turned to 0
      - we then bitmask OR (|) the entire header with (p1)(p2)(p3)(p4)-(p5)(p6)(p7)(p8)-(p9)(p10)00-0000-0000-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the prevInBin representation
*/
void setPrevInBin(U32 *mem_block, U32 *prevInBin){
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
/*- To get the pointer to the next block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
*/
U32 *getNextInBin( U32 *mem_block){
    U32 nextInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    nextInBinReferenceAddress = ((*(mem_block + 1)) & 4190208) >> 12;

    //To get the actual next address out of this, we must multiply prevInBinReferenceAddress by 32 and add our base address to it
    if((base_address + nextInBinReferenceAddress*(32/4)) == mem_block){
        return NULL;
    }
    return (base_address + nextInBinReferenceAddress*(32/4));
}

//function to set the pointer to the next block in bin in the header of the block passed in as a parameter
/*we must take nextInBin and make have it represented through 10 bits
      - we are interested in making bits 11-20 in the second u32 integer to the value of nextInBin
      - we convert nextInBin to an integer, then we subtract the base address, then we divide by 32
        (this gets us the reference address) and then we shift it to the left by 12
      - we then bitmask AND (&) the entire second u32 integer with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the second U32 integer are kept the same and nextInBin is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInBin representation
*/
void setNextInBin(U32 *mem_block, U32 *nextInBin){
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
/*- To get the pointer to the prev block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 1-10
      - To do this, we bitmask using AND (&) with 1111-1111-1100-0000-0000-0000-0000-0000 = 4290772992 in decimal
      - Then, we bit-shift to the right by 22
*/
U32 getPrevInBinReferenceAddress( U32 *mem_block){
    U32 prevInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    return (((*(mem_block + 1)) & 4290772992) >> 22);
}

//function to set the reference address to the previous block in bin in the 2nd U32 integer of the block passed in as a parameter
/*we must take prevInBin and have it represented through 10 bits
      - we bitmask AND (&) the entire header_part2 with 0000-0000-0011-1111-1111-1111-1111-1111 = 4194303 in decimal
        - this ensures that all values in the header_part2 are kept the same and prevInBin is turned to 0
      - we then bitmask OR (|) the entire header with prevInBinRefAddress shifted left by 22
*/
void setPrevInBinReferenceAddress(U32 *mem_block, U32 prevInBinRefAddress){
    U32 header_part2;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4194303;

    header_part2 = header_part2 | (prevInBinRefAddress << 22);
    *(mem_block + 1) = header_part2;
}

//function to retrieve the reference address (between 0 and 1023) to the next block in a bin from the 2nd U32 integer of the block passed in as a parameter
//NOTE: Unlike the getNextInBin function, this function DOES NOT/CANNOT return NULL if the next pointer is pointing to itself (mem_block)
/*- To get the pointer to the next block in the bin, we are interested in the 2nd U32 int in mem_block
      - Specifically, we are interested in bits 11-20
      - To do this, we bitmask using AND (&) with 0000-0000-0011-1111-1111-0000-0000-0000 = 4190208 in decimal
      - Then, we bit-shift to the right by 12
*/
U32 getNextInBinReferenceAddress( U32 *mem_block){
    U32 nextInBinReferenceAddress; //the address stored in the 10-bits is a reference from 0 to 1023, representing which 32-byte block in memory it is pointing to

    return (((*(mem_block + 1)) & 4190208) >> 12);
}

//function to set the reference address to the next block in bin in the 2nd U32 integer of the block passed in as a parameter
/*we must take nextInBin and have it represented through 10 bits
      - we bitmask AND (&) the entire second u32 integer with 1111-1111-1100-0000-0000-1111-1111-1111 = 4290777087 in decimal
        - this ensures that all values in the second U32 integer are kept the same and nextInBin is turned to 0
      - we then bitmask OR (|) the entire header with 0000-0000-00(p1)(p2)-(p3)(p4)(p5)(p6)-(p7)(p8)(p9)(p10)-0000-0000-0000
        where (p1) to (p10) represent the 10 individual bits in the nextInBin representation
*/
void setNextInBinReferenceAddress(U32 *mem_block, U32 nextInBinRefAddress){
    U32 header_part2;

    header_part2 = *(mem_block + 1);
    header_part2 = header_part2 & 4290777087;

    header_part2 = header_part2 | (nextInBinRefAddress << 12);
    *(mem_block + 1) = header_part2;
}

void half_alloc_helper(U32 *block_addr, U32 bin_index){
    U32 *nextBlockInBin;
    U32 *prevBlockInBin;

    //case 1: we are allocating the first block available in a bin
    if (getPrevInBin(block_addr) == NULL){
        //update the bin so that it it now points to the next block in that bin
        bin_ptrs [bin_index] = getNextInBin(block_addr);
        //update the previous ptr of the next block in the bin so that it points to itself
        if(bin_ptrs [bin_index] != NULL){
            setPrevInBin(bin_ptrs [bin_index], bin_ptrs [bin_index]);
        }
    } else{ //case 2: we are NOT allocating the first block in the bin
        //we can check to see if it is the last block in the bin that we are allocating
        if(getNextInBin(block_addr) == NULL){
            prevBlockInBin = getPrevInBin(block_addr);
            //update the nextInBin of prevBlockInBin to point to itself
            setNextInBin(prevBlockInBin, prevBlockInBin);
        } else { //we are allocating something from the middle of our linked list
            prevBlockInBin = getPrevInBin(block_addr);
            nextBlockInBin = getNextInBin(block_addr);

            //update the nextInBin of prevBlockInBin to point to nextBlockInBin
            setNextInBin(prevBlockInBin, nextBlockInBin);
            //update the prevInBin of the nextBlockInBin to point to prevBlockInBin
            setPrevInBin(nextBlockInBin, prevBlockInBin);
        }
    }
    //set the allocate bit in the header to 1
    setAllocate(block_addr);
}

U32 *half_alloc( U32 n) {
    U32 bin_index;
    U32 *current_block;
    U32 *mem_block;
    U32 mem_block_size; //size of the available memory block
    U32 size_of_left_split;
    U32 size_of_right_split;
    U32 *left_block;
    U32 *right_block;
    U32 *nextBlockInBin;
    U32 index_of_bin_to_add_to;
    U32 *nextBlockInNewBin;
    U32 *nextInMemory;
    U32 n_of_bin_to_place_in;

    n += 4; //including the size of the mandatory 4 byte header; also, we add 4 as n represents the number of bytes

    //Determining the index of the bin that we want to start looking in
    if(n <= 32)
    {
        bin_index = 0;
    }
    else
    {
        bin_index = ceil(log(n)/log(2)) - 6;

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

    /*
       For traversal to the appropriate memory block:
       - start at the bin indicated by the bin index
       - if that bin is empty, move to the next bin;
         else traverse the bin and look for a block that is large enough
    */

    mem_block = NULL; //if mem_block remains NULL after the traversal, we know that there is no memory available

    for (bin_index; bin_index < 11; bin_index++){
        if(bin_ptrs[bin_index] != NULL){
            //start traversing the bin to look for an available memory block
            current_block = bin_ptrs[bin_index];

            if(((getSize(current_block))*32) >= n){
                mem_block = current_block;
                break; //leave the loop, we have found our block of memory
            }

            //traverse the rest of the bin because we know the size of current_block is not large enough for our requirements
            while(getNextInBin(current_block) != NULL){
                //check size first to see if it is large enough
                if((getSize(getNextInBin(current_block)))*32 >= n){
                    mem_block = current_block;
                    break;
                } else {
                    current_block = getNextInBin(current_block);
                }
            } //we have went through the bin and found no block that is large enough
        } //else go to the next available bin
    }

    if(mem_block == NULL){
        return NULL; //no memory available
    }

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
            - we DO NOT care about the next and prev bin ptrs for the block we are allocating because those values will be overwritten
            - update the previous ptr of the next block in the memory to point to itself
            - update the bit in the header that indicates that the block is allocated
            - allocate the block in that bin
        --> When we split a block of memory into two blocks as mentioned above, we must:
            - update the next and previous memory ptrs each block before or after the two new blocks
            - update the bin that the original block (the block before we split it) came from so that it now points to the
              next block in that bin
            - initialize the header for each of the two new blocks
            - allocate the first block that we split
            - find the appropriate bin for the 2nd block and add it to the top of that bin
              - if the bin is empty, set the 2nd block to the bin
              - if the bin is not empty, set the prev of the 1st block in the bin to point to our block that wants to be added (2nd block)
                 - then make the nextInBin of our 2nd block to be added to point to the first block in the bin
                 - make the bin pointer now point to the block we just added to the front of the linked list
    */

    // Now to perform the appropriate memory allocations
    //Retrieving the size of the memory block we will be allocating/splitting
    mem_block_size = getSize(mem_block); //size is between 1 and 1024

    if ((mem_block_size*32 - n) >= 32) {
        //split the block into two and then allocate it
        size_of_left_split = (ceil(n/32.0))*(32/4); //should be a multiple of 8
        size_of_right_split = mem_block_size*(32/4) - size_of_left_split; //should be a multiple of 8

        //mem_block is the left block in our split
        right_block = mem_block + size_of_left_split;

    /* We want to first split the right block off into a new bin and then simply call the half_alloc helper on the mem_block
    */
        //update right_block prevInMemory to mem_block
        setPrevInMemory(right_block, mem_block);
        //update right_block nextInMemory to what was left_block's nextInMemory
        nextInMemory = getNextInMemory(mem_block);
        if(nextInMemory != NULL){
            setNextInMemory(right_block, nextInMemory);
        } else {
            setNextInMemory(right_block, right_block); //setting it to point to itself, if it is at the end
        }

        //update size of right_block
        setSize(right_block, ((size_of_right_split*4)/32));

        //set the allocation bit to 0 to ensure that it is deallocated
        setDeallocate(right_block);

        //update mem_block nextInMemory to right_block
        setNextInMemory(mem_block, right_block);
        //update left_block size to size_of_left_split/32
        setSize(mem_block, ((size_of_left_split*4)/32));

        //the right_block must now be placed into a bin of the appropriate size
        n_of_bin_to_place_in = size_of_right_split*4;
        if(n_of_bin_to_place_in <= 32) {
            index_of_bin_to_add_to = 0;
        } else{
            index_of_bin_to_add_to = ceil(log(n_of_bin_to_place_in)/log(2)) - 6; //this index will always be between 0 and 10
        }

        setPrevInBin(right_block, right_block); //we will ALWAYS be adding to the beginning of a block
        //empty bin
        if(bin_ptrs[index_of_bin_to_add_to] == NULL){
            bin_ptrs[index_of_bin_to_add_to] = right_block;
            setNextInBin(right_block, right_block); //because we added to an empty bin
        } else{ //non-empty bin
            setNextInBin(right_block, bin_ptrs[index_of_bin_to_add_to]);
            //update the prevInBin of the first block in the bin to point to right block
            setPrevInBin(bin_ptrs[index_of_bin_to_add_to], right_block);
            //update bin_ptrs to point to the first block in memory
            bin_ptrs [index_of_bin_to_add_to] = right_block;
        }
    }
    half_alloc_helper (mem_block, bin_index);
    return mem_block;
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
        setPrevInBin(block_being_inserted, block_being_inserted); // since this is the only block in the bin, we will set the pointer to the previous block to point to itself
        setNextInBin(block_being_inserted, block_being_inserted); // since this is the only block in the bin, we will set the pointer to the next block to point to itself
    }
    else
    {
        setPrevInBin(bin_ptrs[bin_index], block_being_inserted); //set the original first block's previous pointer in the bin to point to the new block we are inserting
        setNextInBin(block_being_inserted, bin_ptrs[bin_index]); //set the new block's (the one we are inserting) next pointer in the bin to point to the previous first block in the bin
    }
    bin_ptrs[bin_index] = block_being_inserted; // update the bin_ptr in the appropriate index to now point to the new block at the front of the bin
}

//this helper function will update the appropriate bin when passing in the pointer to the address of the block in main memory that needs to be updated
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


    //Checks if the there is only one giant block of allocated memory that is taking up all of memory
    if(getSize(free_block) == 1024)
    {
        insert_block_in_bin(free_block, 10); // insert the block in the front of the appropriate bin
        setNextInMemory(free_block, free_block) ;
        setDeallocate(free_block); // sets the allocated bit to 0
    }

    //there is only one block in memory that is allocated and the rest is free, unallocated memory
    else if( (address_of_prev_block_in_memory == NULL) && (getNextInMemory(address_of_next_block_in_memory) == NULL) ) // there is only one allocated block
    {
        setPrevInMemory(free_block, free_block);
        setPrevInMemory(address_of_next_block_in_memory, address_of_next_block_in_memory);
        setNextInMemory(free_block, free_block);
        setSize(free_block, 1024); //the new size will be the whole of memory since the block to be freed is the only one in memory
        //getNextInMemory((free_block));
        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored the block
        update_bin_when_reallocating(free_block);
        insert_block_in_bin(free_block, 10); // insert the block in the front of the appropriate bin
        update_bin_when_reallocating(address_of_next_block_in_memory);
    }
    // ***** 1. FIRST CASE WHERE THE BLOCK TO BE FREED IS STANDALONE (no free blocks next to it) ***** \\

    else if( ((address_of_prev_block_in_memory == NULL) && (!(isAllocated(address_of_next_block_in_memory)))) || ((address_of_next_block_in_memory == NULL) && (!(isAllocated(address_of_prev_block_in_memory))))
        || ((!(isAllocated(address_of_prev_block_in_memory))) && (!(isAllocated(address_of_next_block_in_memory)))) )
    {
        new_bin_index = floor(log(getSize(free_block)) / log(2)); //the appropriate bin that the block to be freed must be put into

        // if there are no adjacent free blocks, we simply need to update the bin that the freed block must go into
        insert_block_in_bin(address_of_prev_block_in_memory, new_bin_index); // insert the block in the front of the appropriate bin
        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored the block
        //printf("")
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
            printf("The size of the new block is: %d \n", getSize(address_of_prev_block_in_memory));
        }

        setDeallocate(free_block); //set the bit indicating allocation to 0, we dont care about the memory previously stored in the block
    }


}
