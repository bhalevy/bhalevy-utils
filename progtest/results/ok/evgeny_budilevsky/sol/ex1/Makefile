ALL:=sg_copy.h sg_copy.c test.c 
run_test: $(ALL) 
	g++ -g -Wall test.c sg_copy.c -o run_test
test:
	./run_test
clean:
	\rm  run_test
