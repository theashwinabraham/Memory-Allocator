lib:
	gcc -O3 -g -Wall -Wextra -Wpedantic -Werror Memory.c -c -fPIC -o Memory.o
	gcc -O3 -g -shared -o libmem.so Memory.o
	rm Memory.o

test:
	gcc -O3 -g -c -Wall -Wextra -Wpedantic -Werror test.c -o test.o
	gcc -O3 -g -L/home/ashwinabraham/GitHub/Memory-Allocator -lmem test.o -Wl,-rpath=/home/ashwinabraham/GitHub/Memory-Allocator -o test
	rm test.o