PREFIX=/usr/local
CFLAGS=-Wall -g -g -O2 -DPREFIX="\"${PREFIX}\"" -fPIC
LDFLAGS= -ldl -lpthread -lnuma

OBJECTS = pin.o 

.PHONY: all clean
all: makefile.dep pinthreads pin.so

makefile.dep: *.[Cch]
	for i in *.[Cc]; do gcc -MM "$${i}" ${CFLAGS}; done > $@
	
-include makefile.dep

pin.so: pin.o parse_args.o shm.o
	${CC} -shared -o $@ $^ ${LDFLAGS}

pinthreads: pinthreads.o parse_args.o shm.o
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}


install:
	mkdir -p ${PREFIX}/lib/pinthreads/
	cp pin.so ${PREFIX}/lib/pinthreads/
	cp pinthreads ${PREFIX}/bin

clean:
	rm -f *.o *.so makefile.dep pinthreads
