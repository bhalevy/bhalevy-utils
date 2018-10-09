PREP_TARGET=clantlr_prep
PREP_SOURCES=clantlr_prep.c
PREP_OBJECTS=clantlr_prep.o

CLANTLR_HEADERS=intern.h
CLANTLR_SOURCES=clantlr.y clantlr-lex.l
CLANTLR_OBJECTS=clantlr.o clantlr-lex.o
CLANTLR_TARGET=clantlr

TARGETS=$(PREP_TARGET) $(CLANTLR_TARGET)
OBJECTS=$(PREP_OBJECTS) $(CLANTLR_OBJECTS)

CC=gcc
CFLAGS=-Wall -Werror -Wno-unused-function
#LDFLAGS=-g

.PHONY: all
all: $(TARGETS)

${PREP_TARGET}: ${PREP_OBJECTS}
	${CC} ${LDFLAGS} -o $@ $^ $>

${CLANTLR_TARGET}: ${CLANTLR_OBJECTS}
	${CC} ${LDFLAGS} -o $@ $^ $>

clantlr-lex.c: clantlr-lex.l
	flex -o$@ $<

.y.o:
	rm -f y.output
	byacc -vd $< |& tee y.errors && test `wc -l < y.errors` -eq 0
	$(CC) $(CFLAGS) -o $@ -c y.tab.c
	rm y.tab.c
#	mv y.tab.h $*.h
#	mv y.tab.c $*.c

#.c.o:
#	gcc ${CFLAGS} -o $@ -Wall -c $<

clantlr.tgz: $(SOURCES) $(HEADERS) Makefile
	tar czf $@ $^

.PHONY: clean
clean:
	rm -f y.tab.[ch] y.output y.errors
	rm -f clantlr.c clantlr.h clantlr-lex.c *.o $(TARGETS)
