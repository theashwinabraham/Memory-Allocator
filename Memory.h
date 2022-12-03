/*
 * AUTHOR: ASHWIN ABRAHAM
*/
#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>

/* 
 * arg - size of memory block
 * returns - pointer to memory block (if arg == 0, returns NULL)
*/
void *Malloc(size_t);

/*
 * arg1 - number of elements in array
 * arg2 - size of each element
 * returns - pointer to zeroed out array (if either arg is 0, returns NULL)
*/
void *Calloc(size_t, size_t);

/*
 * arg1 - pointer to block to be reallocated (must have been returned by Malloc, Calloc, Realloc or Reallocarray (or) NULL)
 * arg2 - new size of the block
 * returns - pointer to the new block (maybe the same as the old ptr) with values from the old block upto min(new size, old size)
 * if arg1 is NULL, is identical to (and returns) Malloc(arg2)
 * if arg1 is not NULL, and arg2 is 0, is identical to Free(arg1) and returns NULL
 * 
 * In place growth is done whenever possible. In this case arg1 is returned.
 * If in place growth is not possible, the memory is copied, and the old memory is freed
 * Therefore, the pointer passed to this function (arg1) cannot be reused
*/
void *Realloc(void*, size_t);

/*
 * arg1 - pointer to array to be reallocated (must have been returned by Malloc, Calloc, Realloc or Reallocarray (or) NULL)
 * arg2 - new number of elements in the array
 * arg3 - size of each element
 * returns - pointer to the new array (maybe same as the old ptr) with values from the old array upto min(new size, old size)
 * rest of the new memory is zeroed out
 * if arg1 is NULL, is identical to (and returns) Calloc(arg2, arg3)
 * if arg1 is not NULL and either arg2 or arg3 are 0, then is identical to Free(arg1) and returns NULL
 * 
 * In place growth is done whenever possible. In this case arg1 is returned.
 * If in place growth is not possible, the memory is copied, and the old memory is freed
 * Therefore, the pointer passed to this function (arg1) cannot be reused
*/
void *Reallocarray(void*, size_t, size_t);

/*
 * arg1 - pointer to memory block to be freed (must have been returned by Malloc, Calloc, Realloc or Reallocarray (or) NULL)
 * if the argument is NULL, no memory is freed (function just returns)
*/
void Free(void*);

#endif
