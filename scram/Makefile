
SOURCES=scrasm.c
TARGET=scram
OBJECTS=scram.o
LIB=-lssl

${TARGET}: ${OBJECTS}
	gcc -o $@ $^ $> ${LIB}

.c.o:
	gcc -o $@ -Wall -c $<

clean:
	rm -f $(OBJECTS) ${TARGET}
