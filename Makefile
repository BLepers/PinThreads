CFLAGS=-Wall -g -ggdb3 -O0 
LDFLAGS=

OBJECTS = pin.o 

.PHONY: all clean
all: makefile.dep pin.so

makefile.dep: *.[Cch]
	for i in *.[Cc]; do gcc -MM "$${i}" ${CFLAGS}; done > $@
	
-include makefile.dep

pin.so: pin.c
	g++ -fPIC ${CFLAGS} -c pin.c
	g++ -shared -o pin.so pin.o -ldl -lpthread
	#g++ -shared -Wl,-soname,libpmalloc.so -o ldlib.so ldlib.o -ldl -lpthread

clean:
	rm -f *.o *.so makefile.dep

