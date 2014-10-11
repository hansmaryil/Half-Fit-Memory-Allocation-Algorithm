#ifndef HALF_FIT_H_
#define HALF_FIT_H_


typedef unsigned int       U32;

typedef struct mem_block {
    char *base_address;
} mem_block_t;

typedef struct half_fit {
    mem_block_t *mem_block;
} half_fit_t;

void  half_init( void );
char *half_alloc( U32 );
// or void *half_alloc( unsigned int );
void  half_free( char * );

#endif
