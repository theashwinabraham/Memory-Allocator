# Memory Allocator

This is a simple memory allocator written in the C programming language.
The functions ```malloc```, ```calloc```, ```realloc```, ```reallocarray``` and ```free``` have been implemented in the
file ```Memory.c```. The names of these functions written are the same as the names of the standard functions, except the first letter has
been capitalized. The behaviour of these functions is the same as the standard functions.

These functions will be compiled into a Shared Library which can be loaded by programs at runtime. A makefile has been provided to compile the library. The command to compile the library is:

    make lib

A test file (```test.c```) has also been provided.
