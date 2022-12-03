/*
 * AUTHOR: ASHWIN ABRAHAM
*/
#include <stdio.h>
#include "Memory.h"
#include <unistd.h>
// #include <stdlib.h>

int *buff = NULL;
int calc_size = 0;
int alloc_size = 0;

int *get_el(int i)
{
    if(i >= alloc_size)
    {
        if(i % 2)
        {
            int new_alloc_size = 2*i + 1;
            int *nbuff = Malloc(new_alloc_size*sizeof(int));
            for(int i = 0; i<alloc_size; ++i) nbuff[i] = buff[i];
            Free(buff);
            buff = nbuff;
            alloc_size = new_alloc_size;
        }
        else
        {
            alloc_size = 2*i + 1;
            buff = Realloc(buff, alloc_size*sizeof(int));
        }
    }
    if(i%7 == 0) Malloc(2048 + 103*i); 
    return buff + i;
}

int memo_fib(int n)
{
    if(n < 0) return 0;
    if(n >= calc_size)
    {
        int val = memo_fib(n - 1) + memo_fib(n - 2);
        *get_el(n) = val;
        calc_size = n + 1;
    }
    return *get_el(n);
}

int main()
{
    *get_el(0) = 0;
    *get_el(1) = 1;
    calc_size = 2;

    int val;
    while(scanf("%d", &val) == 1)
    {
        printf("Memoized Fibonacci (%d): %d\n", val, memo_fib(val));
    }
    sleep(10);
}
