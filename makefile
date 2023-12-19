compiler=gcc
flags=-std=gnu11 -march=native -Wall -Wno-missing-braces -O3

# The 64-bit version of this program is faster but only supports graphs up to 64 vertices.
64bit: lonelyEdges.c readGraph/readGraph6.c bitset.h 
	$(compiler) -DUSE_64_BIT -o lonelyEdges lonelyEdges.c readGraph/readGraph6.c $(flags)

# There are two different implementations of the 128-bit version. The array version generally performs faster.
128bit: lonelyEdges.c readGraph/readGraph6.c bitset.h 
	$(compiler) -DUSE_128_BIT -o lonelyEdges-128 lonelyEdges.c readGraph/readGraph6.c $(flags)

128bitarray: lonelyEdges.c readGraph/readGraph6.c bitset.h 
	$(compiler) -DUSE_128_BIT_ARRAY -o lonelyEdges-128a lonelyEdges.c readGraph/readGraph6.c $(flags)	

profile: lonelyEdges.c readGraph/readGraph6.c bitset.h 
	$(compiler) -DUSE_64_BIT -o lonelyEdges-pr lonelyEdges.c readGraph/readGraph6.c -std=gnu11 -march=native -Wall -Wno-missing-braces -g -pg

all: 64bit 128bit 128bitarray

.PHONY: clean
clean:
	rm -f lonelyEdges lonelyEdges-128 lonelyEdges-128a lonelyEdges-pr gmon.out