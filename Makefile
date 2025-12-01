# ssdsim linux support
all:ssd 
	
clean:
	rm -f ssd *.o *~
.PHONY: clean

ssd: ssd.o avlTree.o flash.o initialize.o pagemap.o LRU.o    
	gcc -g -o ssd ssd.o avlTree.o flash.o initialize.o pagemap.o LRU.o
ssd.o: flash.h initialize.h pagemap.h LRU.h
	gcc -c -g -O3 ssd.c
flash.o: pagemap.h
	gcc -c -g -O3 flash.c
initialize.o: avlTree.h pagemap.h
	gcc -c -g -O3 initialize.c
pagemap.o: initialize.h
	gcc -c -g -O3 pagemap.c
avlTree.o: 
	gcc -c -g -O3 avlTree.c
LRU.o:LRU.h
	gcc -c -g -O3 LRU.c

