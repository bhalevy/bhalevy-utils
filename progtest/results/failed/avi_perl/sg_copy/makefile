all: sgtest

sgtest: sg_copy.o sg_copy_main.c
	gcc -o sgtest sg_copy.o sg_copy_main.c

sg_copy.o: sg_copy.h sg_copy.c
	gcc -c sg_copy.c -D DEBUG

clean:
	rm -rf *.o sgtest


