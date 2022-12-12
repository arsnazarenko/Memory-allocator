#define _GNU_SOURCE
#include <unistd.h>
#include <stddef.h>
#include "mem_internals.h"
#include "mem.h"
#include "util.h"
#include <sys/mman.h>

void debug_block(struct block_header* b, const char* fmt, ... );
void debug(const char* fmt, ... );

extern inline block_size size_from_capacity( block_capacity cap );
extern inline block_capacity capacity_from_size( block_size sz );

static bool            block_is_big_enough( size_t query, struct block_header* block ) { return block->capacity.bytes >= query; }
static size_t          pages_count   ( size_t mem )                      { return mem / getpagesize() + ((mem % getpagesize()) > 0); }
static size_t          round_pages   ( size_t mem )                      { return getpagesize() * pages_count( mem ) ; }

static void block_init( void* restrict addr, block_size block_sz, void* restrict next ) {
  *((struct block_header*)addr) = (struct block_header) {
    .next = next,
    .capacity = capacity_from_size(block_sz),
    .is_free = true
  };
}

static size_t region_actual_size( size_t query ) { return size_max( round_pages( query ), REGION_MIN_SIZE ); }

extern inline bool region_is_invalid( const struct region* r );



static void* map_pages(void const* addr, const size_t length, const int additional_flags) {
  return mmap( (void*) addr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | additional_flags , 0, 0 );
}

/*  аллоцировать регион памяти и инициализировать его блоком */
static struct region alloc_region  ( void const * addr,const size_t query ) {
    const size_t region_size = region_actual_size(query);
    bool extends;
    void* new_adrr = NULL;
    if ( (new_adrr = map_pages(addr, region_size, MAP_FIXED_NOREPLACE) ) != MAP_FAILED) {
        extends = true;
    } else if ( (new_adrr = map_pages(NULL, region_size, 0)) != MAP_FAILED) {
        extends = false;
    } else {
        return REGION_INVALID;
    }
    struct region result_region = (struct region) { .addr = new_adrr, .size = region_size, .extends = extends};
    block_init(new_adrr, (block_size) {region_size}, NULL);
    return result_region;
}

static void* block_after( struct block_header const* block );

void* heap_init( const size_t initial ) {
  const struct region region = alloc_region( HEAP_START, initial );
  if ( region_is_invalid(&region) ) return NULL;
  return region.addr;
}

#define BLOCK_MIN_CAPACITY 24

/*  --- Разделение блоков (если найденный свободный блок слишком большой )--- */

static bool block_splittable( struct block_header* restrict block, const size_t query) {
  return block-> is_free && query + offsetof( struct block_header, contents ) + BLOCK_MIN_CAPACITY <= block->capacity.bytes;
}

static bool split_if_too_big( struct block_header* block, const size_t query ) {
    if (block_splittable(block, query)) {
        const block_size next_block_size = (block_size) {block->capacity.bytes - query};
        block->capacity = (block_capacity) {query};
        struct block_header* const next_block_start = (struct block_header *) block_after(block);
        block_init(next_block_start, next_block_size, block->next);
        block->next = next_block_start;
        return true;
    }
    return false;
}


/*  --- Слияние соседних свободных блоков --- */

static void* block_after( struct block_header const* block ) {
  return  (void*) (block->contents + block->capacity.bytes);
}
static bool blocks_continuous (
                               struct block_header const* fst,
                               struct block_header const* snd ) {
  return (void*)snd == block_after(fst);
}

static bool mergeable(struct block_header const* restrict fst, struct block_header const* restrict snd) {
  return fst->is_free && snd->is_free && blocks_continuous( fst, snd ) ;
}

static bool try_merge_with_next( struct block_header* block ) {
  if (block && block-> next && mergeable(block, block->next)) {
      block->capacity.bytes += size_from_capacity(block->next->capacity).bytes;
      block->next = block->next->next;
      return true;
  }
  return false;
}
//  block - pointer to start block of this sequence
void try_merge_block_sequence(struct block_header* block) {
    while(try_merge_with_next(block));
}


/*  --- ... ecли размера кучи хватает --- */

struct block_search_result {
  enum {BSR_FOUND_GOOD_BLOCK, BSR_REACHED_END_NOT_FOUND, BSR_CORRUPTED} type;
  struct block_header* block;
};

static struct block_search_result find_good_or_last  ( struct block_header* restrict block, const size_t sz )    {
    struct block_header* cur_block = block;
    while(cur_block) {
        try_merge_block_sequence(cur_block);
        if (cur_block->is_free && block_is_big_enough(sz, cur_block)) {
            return (struct block_search_result) {.type = BSR_FOUND_GOOD_BLOCK,.block = cur_block };
        }
        if (cur_block->next) { cur_block = cur_block->next; }
        else { break; }
    }
    return (struct block_search_result) {.type = BSR_REACHED_END_NOT_FOUND, cur_block};
}

/*  Попробовать выделить память в куче начиная с блока `block` не пытаясь расширить кучу
 Можно переиспользовать как только кучу расширили. */
static struct block_search_result try_memalloc_existing ( const size_t query, struct block_header* block ) {
    struct block_search_result res = find_good_or_last(block, query);
    if ( res.type == BSR_FOUND_GOOD_BLOCK ) {
        split_if_too_big(res.block, query);
        res.block->is_free = false;
    }
    return res;
}


static struct block_header* grow_heap( struct block_header* restrict last, size_t query ) {
    // we should alloc new region for [header] + [capacity] (query == capacity)
    size_t block_mem_size = size_from_capacity((block_capacity){query}).bytes;
    void* const next_block_addr = block_after(last);
    const struct region new_reg = alloc_region(next_block_addr, block_mem_size);
    if (!region_is_invalid(&new_reg)) {
        last->next = new_reg.addr;
        return new_reg.addr;
    }
    return NULL;
}

/*  Реализует основную логику malloc и возвращает заголовок выделенного блока */
static struct block_header* memalloc( size_t query, struct block_header* heap_start) {
    const size_t actual_query = size_max(query, BLOCK_MIN_CAPACITY);
    struct block_search_result bsr = try_memalloc_existing(actual_query, heap_start);
    if (bsr.type != BSR_FOUND_GOOD_BLOCK) {
        struct block_header* new_reg_header = grow_heap(bsr.block, actual_query);
        // if we have new mem region
        if (new_reg_header) {
            //  we already merge all bloch in prev region and not found good block
            //  now, we can start check and try merge blocks from last reached block of previous region
            struct block_search_result result = try_memalloc_existing(actual_query, bsr.block);
            return result.block;
        }
        // if we have cannot allocate new region
        return NULL;
    }
    return  bsr.block;
}

void* _malloc( size_t query ) {
  struct block_header* const addr = memalloc( query, (struct block_header*) HEAP_START );
  if (addr) return addr->contents;
  else return NULL;
}

static struct block_header* block_get_header(void* contents) {
  return (struct block_header*) (((uint8_t*)contents)-offsetof(struct block_header, contents));
}

void _free( void* mem ) {
  if (!mem) return;
  struct block_header* header = block_get_header( mem );
  header->is_free = true;
  try_merge_block_sequence(header);
}
