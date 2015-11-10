CC=gcc
CFLAGS=-I. -Wall -g
DEPS = sg_copy.h
OBJ = main.o sg_copy.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

sg_list: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm *o sg_list
