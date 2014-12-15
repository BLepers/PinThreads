PREFIX=/usr/local
CFLAGS=-Wall -g -g -O2 -DPREFIX="\"${PREFIX}\"" -fPIC
LDLIBS= -ldl -lpthread -lnuma

.PHONY: all clean
all: makefile.dep pinthreads pin.so pinhook.so tests/testhooks tests/delayedthreads tests/multipleprocesses

makefile.dep: *.[Cch]
	for i in *.[Cc]; do ${CC} -MM "$${i}" ${CFLAGS}; done > $@

-include makefile.dep

pin.so: pin.o parse_args.o shm.o pinserver.o
	${CC} -shared -o $@ $^ ${LDLIBS}

pinthreads: pinthreads.o parse_args.o shm.o
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

pinhook.so: pinhook.o
	${CC} -shared -o $@ $^ ${LDLIBS}

tests/testhooks: tests/testhooks.o pinhook.so
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS} pinhook.so

tests/delayedthreads: tests/delayedthreads.o
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

tests/multipleprocesses: tests/multipleprocesses.o
	${CC} ${CFLAGS} -o $@ $^ ${LDLIBS}

install:
	mkdir -p ${PREFIX}/lib/pinthreads/
	cp pin.so ${PREFIX}/lib/pinthreads/
	cp pinthreads ${PREFIX}/bin

clean:
	rm -f *.o *.so makefile.dep pinthreads tests/testhooks tests/delayedthreads tests/multipleprocesses tests/*.o
