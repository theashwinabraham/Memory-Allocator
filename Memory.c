/*
 * AUTHOR: ASHWIN ABRAHAM
*/
#include "Memory.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define ALIGNMENT 8 // must be a power of 2 between less PAGESIZE
#define PAGESIZE ((size_t) sysconf(_SC_PAGESIZE)) // Kernel Pagesize - mmap allocates in multiples of pagesize


static inline size_t roundup(size_t a, size_t b) // rounds up a to a multiple of b
{
    if(a % b == 0) return a;
    return (1 + a/b)*b;
}

// is pointer aligned (8 byte aligned)
// sizeof(memory_header) must be divisible by ALIGNMENT
typedef struct memory_header {
    struct memory_header *page_header; // pointer to the start of the mapped set of pages containing the pool
    struct memory_header *prev_block; // pointer to the previous memory pool
    struct memory_header *next_block; // pointer to the next memory pool
    size_t size; // amount of memory requested
    size_t total_size; // total size of the memory pool
} memory_header;


// head and tail of linked list of memory pools
static memory_header *head = NULL;
static memory_header *tail = NULL;


// TODO: Make everything overflow safe


// memory requested from kernel via mmap
// first fit algorithm is used (search for the first free pool whose size >= requested size)
// if not found, allocate memory with mmap
void *Malloc(size_t sz)
{
    if(sz == 0) return NULL;
    if(head == NULL) // no allocated blocks (tail must also be NULL)
    {
        head = mmap(NULL, sizeof(memory_header) + sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        /*
        * MMAP - returns a pointer to a collection of consecutive pages (allocated memory = 2nd arg rounded up to a multiple of pagesize)
        * 1st arg = NULL denotes that we don't particularly care where the memory is located
        * 3rd arg = we should be able to read from and write to the memory
        * 4th arg = MAP_PRIVATE - the memory is only for the current process, MAP_ANON - we aren't mapping a file into memory
        * 5th, 6th args = specific for memory mapping a file
        */
        head->page_header = head;
        head->prev_block = NULL;
        head->next_block = NULL;
        head->size = sz;
        head->total_size = roundup(sizeof(memory_header) + sz, PAGESIZE);
        tail = head;
        return ((char*) head) + sizeof(memory_header);
    }
    // search for a pool with enough space
    for(memory_header *ptr = head; ptr != NULL; ptr = ptr->next_block)
    {
        // using a free pool
        if(ptr->size == 0 && ptr->total_size >= sizeof(memory_header) + sz)
        {
            ptr->size = sz;
            return ((char*) ptr) + sizeof(memory_header);
        }
        // splitting a pool
        // every split should be atleast sizeof(memory_header) away from a page boundary
        if(ptr->total_size >= roundup(sizeof(memory_header) + ptr->size, ALIGNMENT) + sizeof(memory_header) + sz)
        {
            memory_header *n_block = (memory_header *) (((char *) ptr) + roundup(sizeof(memory_header) + ptr->size, ALIGNMENT));
            if(((uintptr_t) n_block) % PAGESIZE >= sizeof(memory_header))
            {
                n_block->prev_block = ptr;
                n_block->next_block = ptr->next_block;
                if(tail == ptr) tail = n_block;
                else ptr->next_block->prev_block = n_block;
                ptr->next_block = n_block;
                n_block->page_header = ptr->page_header;
                n_block->size = sz;
                n_block->total_size = ptr->total_size - roundup(sizeof(memory_header) + ptr->size, ALIGNMENT);
                ptr->total_size = roundup(sizeof(memory_header) + ptr->size, ALIGNMENT);
                return ((char*) n_block) + sizeof(memory_header);
            }
        }
    }
    // no pools with enough free memory - create a new one
    memory_header *n_block = mmap(NULL, sizeof(memory_header) + sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    n_block->page_header = n_block;
    n_block->prev_block = tail;
    tail->next_block = n_block;
    tail = n_block;
    n_block->total_size = roundup(sizeof(memory_header) + sz, PAGESIZE);
    n_block->size = sz;
    return ((char*) n_block) + sizeof(memory_header);
}

// merge freed pool with previous pool with the same page header
// if previous pool is in a different page, mark pool as freed (size = 0)
// at the end, check if page header of freed block can be (atleast partially) unmapped
// The only pools that straddle pages are the results of mmaps (i.e. have page_header as themselves)
// Pools can be created by mmaps, splitting, or merging
// Merging never creates pools that straddle pages, as we merge only when prev page header is the same
// Splitting never creates pools that straddle pages, because in the initial mmapped memory, every page boundary
// is in the allocated region
// when the result of an mmap that straddles a page boundary is freed, it is entirely unmapped upto the last page boundary
void Free(void *ptr)
{
    if(ptr == NULL) return;
    memory_header* act_ptr = (memory_header*) (((char*) ptr) - sizeof(memory_header));
    if(act_ptr->prev_block == NULL) // act_ptr == head
    {
        if(act_ptr->next_block == NULL) // only one alloc so far (head == tail == act_ptr), which we are freeing
        {
            head = NULL;
            tail = NULL;
            munmap(act_ptr->page_header, act_ptr->total_size);
            return;
        }
        act_ptr->size = 0;
    }
    else
    {
        if(act_ptr->prev_block->page_header == act_ptr->page_header)
        {
            act_ptr->prev_block->total_size += act_ptr->total_size;
            act_ptr->prev_block->next_block = act_ptr->next_block;
            if(act_ptr->next_block != NULL) act_ptr->next_block->prev_block = act_ptr->prev_block;
            else tail = act_ptr->prev_block;
        }
        else
        {
            act_ptr->size = 0;
        }
    }
    memory_header* pgh =  act_ptr->page_header;
    if(pgh->size == 0 && pgh->total_size >= PAGESIZE)
    {
        if(pgh->total_size % PAGESIZE == 0)
        {
            // no need to update page headers in this case
            // because next pool must start on a new page
            // and any pool that starts on a page must have been allocated by mmap
            // therefore, it will have header as itself
            if(pgh->prev_block != NULL) pgh->prev_block->next_block = pgh->next_block;
            else head = pgh->next_block;

            if(pgh->next_block != NULL) pgh->next_block->prev_block = pgh->prev_block;
            else tail = pgh->prev_block;
        }
        else
        {
            // will not overwrite any other blocks, because splits leave atleast sizeof(memory_header) gap from page boundary
            memory_header* n_ptr = (memory_header *)(((char*) pgh) + (pgh->total_size/PAGESIZE)*PAGESIZE);
            n_ptr->prev_block = pgh->prev_block;
            n_ptr->next_block = pgh->next_block;
            n_ptr->size = 0;
            n_ptr->total_size = ((char*) pgh->next_block) - ((char*) n_ptr);
            n_ptr->page_header = pgh;
            for(memory_header *pptr = n_ptr; pptr != NULL && pptr->page_header == pgh; pptr = pptr->next_block) pptr->page_header = n_ptr;


            if(pgh->prev_block != NULL) pgh->prev_block->next_block = n_ptr;
            else head = n_ptr;

            if(pgh->next_block != NULL) pgh->next_block->prev_block = n_ptr;
            else tail = n_ptr;
        }
        munmap(pgh, (pgh->total_size/PAGESIZE)*PAGESIZE);
    }
}

// implementation is very similiar to malloc
// note that the result of mmap with MAP_ANON is always zeroed out
void *Calloc(size_t nmemb, size_t szo)
{
    if(szo == 0 || nmemb == 0) return NULL;
    size_t sz = nmemb*szo;
    if(head == NULL) // no allocated blocks (tail must also be NULL)
    {
        head = mmap(NULL, sizeof(memory_header) + sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        /*
        * MMAP - returns a pointer to a collection of consecutive pages (allocated memory = 2nd arg rounded up to a multiple of pagesize)
        * 1st arg = NULL denotes that we don't particularly care where the memory is located
        * 3rd arg = we should be able to read from and write to the memory
        * 4th arg = MAP_PRIVATE - the memory is only for the current process, MAP_ANON - we aren't mapping a file into memory
        * 5th, 6th args = specific for memory mapping a file
        */
        head->page_header = head;
        head->prev_block = NULL;
        head->next_block = NULL;
        head->size = sz;
        head->total_size = roundup(sizeof(memory_header) + sz, PAGESIZE);
        tail = head;
        return ((char*) head) + sizeof(memory_header); // already zeroed out
    }
    // search for a pool with enough space
    for(memory_header *ptr = head; ptr != NULL; ptr = ptr->next_block)
    {
        // using a free pool
        if(ptr->size == 0 && ptr->total_size >= sizeof(memory_header) + sz)
        {
            ptr->size = sz;
            memset(((char*) ptr) + sizeof(memory_header), 0, sz);
            return ((char*) ptr) + sizeof(memory_header);
        }
        // splitting a pool
        // every split should be atleast sizeof(memory_header) away from a page boundary
        if(ptr->total_size >= roundup(sizeof(memory_header) + ptr->size, ALIGNMENT) + sizeof(memory_header) + sz)
        {
            memory_header *n_block = (memory_header *) (((char *) ptr) + roundup(sizeof(memory_header) + ptr->size, ALIGNMENT));
            if(((uintptr_t) n_block) % PAGESIZE >= sizeof(memory_header))
            {
                n_block->prev_block = ptr;
                n_block->next_block = ptr->next_block;
                if(tail == ptr) tail = n_block;
                else ptr->next_block->prev_block = n_block;
                ptr->next_block = n_block;
                n_block->page_header = ptr->page_header;
                n_block->size = sz;
                n_block->total_size = ptr->total_size - roundup(sizeof(memory_header) + ptr->size, ALIGNMENT);
                ptr->total_size = roundup(sizeof(memory_header) + ptr->size, ALIGNMENT);
                memset(((char*) n_block) + sizeof(memory_header), 0, sz);
                return ((char*) n_block) + sizeof(memory_header);
            }
        }
    }
    // no pools with enough free memory - create a new one
    memory_header *n_block = mmap(NULL, sizeof(memory_header) + sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    n_block->page_header = n_block;
    n_block->prev_block = tail;
    tail->next_block = n_block;
    tail = n_block;
    n_block->total_size = roundup(sizeof(memory_header) + sz, PAGESIZE);
    n_block->size = sz;
    return ((char*) n_block) + sizeof(memory_header); // already zeroed out
}

void *Realloc(void* ptr, size_t nsize)
{
    if(ptr == NULL) return Malloc(nsize);
    if(nsize == 0)
    {
        Free(ptr);
        return NULL;
    }
    memory_header* act_ptr = (memory_header*) (((char*) ptr) - sizeof(memory_header));
    if(nsize <= act_ptr->total_size - sizeof(memory_header))
    {
        act_ptr->size = nsize;
        return ptr;
    }
    char *rptr = (char*) Malloc(nsize);
    memcpy(rptr, (char*) ptr, act_ptr->size); // can use memcpy as newly alloced ptr will never overlap with any previously alloced pointers
    Free(ptr);
    return rptr;
}

void *Reallocarray(void *ptr, size_t nmemb, size_t szo)
{
    if(ptr == NULL) return Calloc(nmemb, szo);
    if(nmemb == 0 || szo == 0)
    {
        Free(ptr);
        return NULL;
    }
    memory_header* act_ptr = (memory_header*) (((char*) ptr) - sizeof(memory_header));
    if(nmemb*szo <= act_ptr->total_size - sizeof(memory_header))
    {
        if(act_ptr->size < nmemb*szo) memset(((char*) ptr) + act_ptr->size, 0, nmemb*szo - act_ptr->size);
        act_ptr->size = nmemb*szo;
        return ptr;
    }
    char *rptr = (char*) Calloc(nmemb, szo);
    memcpy(rptr, (char*) ptr, act_ptr->size); // can use memcpy as newly alloced ptr will never overlap with any previously alloced pointers
    Free(ptr);
    return rptr;
}
