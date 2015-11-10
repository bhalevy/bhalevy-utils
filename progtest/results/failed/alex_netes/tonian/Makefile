CC=gcc
CFLAGS=-I.

all: snippet_1.o snippet_1_test.o snippet_2_test.o snippet_3_test.o
	@$(CC) -o snippet_1_test snippet_1.o snippet_1_test.o
	@$(CC) -o snippet_2_test snippet_2_test.o
	@$(CC) -o snippet_3_test snippet_3_test.o

clean:
	@rm -fr snippet_3_test snippet_2_test snippet_1_test
	@rm -fr *.o
