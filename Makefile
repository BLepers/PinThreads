PREFIX=/usr/local
CFLAGS=-Wall -g -g -O2 -DPREFIX="\"${PREFIX}\""
LDFLAGS=

OBJECTS = pin.o 

.PHONY: all clean
all: makefile.dep pinthreads pin.so

makefile.dep: *.[Cch]
	for i in *.[Cc]; do gcc -MM "$${i}" ${CFLAGS}; done > $@
	
-include makefile.dep

pin.so: pin.c
	${CXX} -fPIC ${CFLAGS} -c pin.c
	${CXX} -shared -o pin.so pin.o -ldl -lpthread

pinthreads: pinthreads.c
	${CC} ${CFLAGS} -o pinthreads pinthreads.c

install:
	mkdir -p ${PREFIX}/lib/pinthreads/
	cp pin.so ${PREFIX}/lib/pinthreads/
	cp pinthreads ${PREFIX}/bin

clean:
	rm -f *.o *.so makefile.dep pinthreads
