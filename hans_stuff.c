/*#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "half_fit.h"
#include "type.h"

//global variable declaration
half_fit_t half_fit;
char *base_address;
U32 init_size = 32768;

void  half_init( void )
{
    base_address = (char *) malloc(init_size);
    printf("Base Address: %d %x\n", base_address, base_address);

    //initializing header (first 4 bytes)
    // both previous and next ptrs in the header are pointing to 0000000000 (pointing to itself)
    *base_address = *base_address >> 8; //set the first 8 bits to 0
    *(base_address + 1) = *(base_address + 1) << 8;//the next 8 bits to 0
    *(base_address + 2) = *(base_address + 2) << 8;// this completes the declaration of previous and next to itself
    //size of 1024 will be represented as 0000000000 because size can be between 1 and 1024 but 1024 requires 11 bits to
    //represent; since block size is minimum 1, we can use 0000000000 to represent it
    *(base_address + 3) = *(base_address + 3) << 8; //6 bits for size, 1 bit for unallocated, 1 bit don't care

    printf("Base Address: %d %x  %d\n", base_address, base_address, *base_address);


}

void *half_alloc( int );
// or void *half_alloc( unsigned int );
void  half_free( void * );
*/
