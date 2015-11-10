
HEADERS=icintern.h
SOURCES=iclex.l ic.y icvar.c
TARGET=ic
OBJECTS=ic.o iclex.o icvar.o
LIB=-lm
CC=gcc
#CFLAGS=-g
#LDFLAGS=-g

${TARGET}: ${OBJECTS}
	${CC} ${LDFLAGS} -o $@ $^ $> ${LIB}

${OBJECTS}: ${HEADERS}

#iclex.c: iclex.l
#	flex -o$@ $<

.y.o:
	byacc -d $<
	$(CC) $(CCFLAGS) -o $@ -c y.tab.c
	rm y.tab.c
#	mv y.tab.h $*.h
#	mv y.tab.c $*.c

#.c.o:
#	gcc ${CFLAGS} -o $@ -Wall -c $<

ic.tgz: $(SOURCES) $(HEADERS) Makefile
	tar czf $@ $^

clean:
	rm -f ic.c ic.h y.tab.[ch] iclex.c $(OBJECTS) ic
