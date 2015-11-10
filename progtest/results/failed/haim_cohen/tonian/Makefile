OUTFILE := tonian
OBJS := main.o snippet_2.o snippet_3.o sg_copy.o

PLATFORM64=1
ifdef PLATFORM64
	X64_FLAGS=-m64
else
	X64_FLAGS=-m32
endif


OPTFLAGS = ${X64_FLAGS} -Wall -D_REENTRANT -Wno-deprecated
CFLAGS = ${OPTFLAGS} ${INCLUDE}

DEBUG_TONIAN=1
ifdef DEBUG_TONIAN
	CFLAGS += -g -O0 -D_DEBUG
else
	CFLAGS += -DNDEBUG
endif

# avoid deleting intermediate files.
.SECONDARY:

# .PHONY is target that not creating file.
.PHONY: all clean
all:	${OUTFILE}

${OUTFILE}: $(OBJS)
	@echo "############### linking everything ... "
	$(CC) $(CFLAGS) -o ${OUTFILE} $(OBJS) ${STATLIBS} $(LIBS)

%.o: %.c
	@echo "############### Building $<"
	$(CC) $(CFLAGS) ${INC_DIRS} -c $< -o $@

clean:
	rm -rf ${OUTFILE} ${OBJS}