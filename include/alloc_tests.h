#ifndef _ALLOC_TESTS_H
#define _ALLOC_TESTS_H

#include "mem_internals.h"
#include "mem.h"
#define HEAP_SIZE_TEST 2000


#define MALLOC_AND_GET_HEADER(num, size) \
void* m##num = _malloc(size);             \
struct block_header* h##num = block_get_header(m##num);

static inline struct block_header* block_get_header(void* contents) {
    return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void run_all_tests();

#endif
