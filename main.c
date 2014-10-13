#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include "half_fit.h"
#include "type.h"
//#include "lpc17xx.h"


int main (void){
    U32 size;
    U32 prevInMemoryRefAddress;
    U32 nextInMemoryRefAddress;
    U32 *prevInMemory;
    U32 *nextInMemory;
    bool allocated;
    U32 *prevInBin;
    U32 *nextInBin;
    U32 prevInBinRefAddress;
    U32 nextInBinRefAddress;
    U32 *temp;

    //Testing the init function
    half_init();
    printf("Base Address: %d %x\n", *base_address, base_address);
    printf("Base Address + 1: %d %x\n", *(base_address + 1), (base_address + 1));

    //Testing the getter functions that retrieve things from the header
    size = getSize(base_address);
    prevInMemoryRefAddress = getPrevInMemoryReferenceAddress(base_address);
    nextInMemoryRefAddress = getNextInMemoryReferenceAddress(base_address);
    prevInMemory = getPrevInMemory(base_address);
    nextInMemory = getNextInMemory(base_address);
    allocated = isAllocated(base_address);

    printf("size: %d \n", size);
    printf("prev In Memory: %d %p \n", prevInMemoryRefAddress, prevInMemory);
    printf("next In Memory: %d %p \n", nextInMemoryRefAddress, nextInMemory);
    printf("allocated: %d \n", allocated);

    //Testing the getter functions that retrieve the next and previous addresses/pointers for a bin
    prevInBin = getPrevInBin(base_address);
    nextInBin = getNextInBin(base_address);
    prevInBinRefAddress = getPrevInBinReferenceAddress(base_address);
    nextInBinRefAddress = getNextInBinReferenceAddress(base_address);

    printf("prev In Bin: %d %p \n", prevInBinRefAddress, prevInBin);
    printf("next In Bin: %d %p \n", nextInBinRefAddress, nextInBin);

    //Testing the setter functions for the header
    setSize(base_address, 1022);
    size = getSize(base_address);
    setPrevInMemory(base_address, (base_address + 8));
    prevInMemory = getPrevInMemory(base_address);
    setNextInMemory(base_address, (base_address + 16));
    nextInMemory = getNextInMemory(base_address);

    printf("updated size: %d \n", size);
    printf("updated prev In Memory: %p %p %p\n", prevInMemory, (base_address + 8), base_address);
    printf("updated next In Memory: %p %p %p\n", nextInMemory, (base_address + 16), base_address);

    setPrevInMemoryReferenceAddress(base_address, 4);
    prevInMemoryRefAddress = getPrevInMemoryReferenceAddress(base_address);
    printf("prev In Memory Reference Address: %d %d \n", prevInMemoryRefAddress, 4);
    setNextInMemoryReferenceAddress(base_address, 550);
    nextInMemoryRefAddress = getNextInMemoryReferenceAddress(base_address);
    printf("next In Memory Reference Address: %d %d \n", nextInMemoryRefAddress, 550);

    setAllocate(base_address);
    allocated = isAllocated();
    printf("allocated updated-set: %d \n", allocated);
    setDeallocate(base_address);
    allocated = isAllocated();
    printf("allocated updated-reset: %d \n", allocated);

    setPrevInBin(base_address, (base_address + 24));
    prevInBin = getPrevInBin(base_address);
    setNextInBin(base_address,(base_address + 32));
    nextInBin = getNextInBin(base_address);

    printf("updated prev In Bin: %p %p %p\n", prevInBin, (base_address + 24), base_address);
    printf("updated next In Bin: %p %p %p\n", nextInBin, (base_address + 32), base_address);

    setPrevInBinReferenceAddress(base_address, 25);
    prevInBinRefAddress = getPrevInBinReferenceAddress(base_address);
    printf("prev In Bin Reference Address: %d %d \n", prevInBinRefAddress, 25);
    setNextInBinReferenceAddress(base_address, 800);
    nextInBinRefAddress = getNextInBinReferenceAddress(base_address);
    printf("next In Bin Reference Address: %d %d \n", nextInBinRefAddress, 800);


    //Testing the getter functions to retrieve the pointers to the previous and next memory blocks in a bin

    //U32 *test = half_alloc(32760);

}
