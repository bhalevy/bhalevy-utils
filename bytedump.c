#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static void putnibble(int ch)
{
	int nibble = ch & 0xf;
	if (nibble < 10) {
		putchar('0' + nibble);
	} else {
		putchar('a' + nibble - 10);
	}
}

enum prefix_type {
	PREFIX_NONE,
	PREFIX_OFFSET,
	PREFIX_COUNT,
};

int
main(int argc, char **argv)
{
	const char *fname = "<stdin>";
	FILE *f = stdin;
	off_t offset = 0;
	size_t i, count = 0;
	const size_t row_len = 16;
	const int offset_width = 8;
	enum prefix_type ptype = PREFIX_OFFSET;
	int ch;

	while ((ch = getopt(argc, argv, ":o:n:p:")) != -1) {
		switch (ch) {
		case 'o':
			offset = strtoll(optarg, 0, 0);
			break;
		case 'n':
			count = strtoll(optarg, 0, 0);
			break;
		case 'p':
			if (!strcmp(optarg, "none")) {
				ptype = PREFIX_NONE;
			} else if (!strcmp(optarg, "offset")) {
				ptype = PREFIX_OFFSET;
			} else if (!strcmp(optarg, "count")) {
				ptype = PREFIX_COUNT;
			} else {
				fprintf(stderr, "%s: Invalid prefix type\n", optarg);
				goto usage;
			}
			break;
		default: /* '?' */ {
			usage:
			fprintf(stderr, "Usage: %s [-o offset] [-n count] [-p (none|offset|count)] [name]\n", argv[0]);
			exit(EXIT_FAILURE);
			}
		}
	}

	if (optind < argc) {
		fname = argv[optind];
		f = fopen(fname, "r");
		if (!f) {
			fprintf(stderr, "Could not open %s: %s\n", fname, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (fseek(f, offset, SEEK_SET)) {
		fprintf(stderr, "Could not seek %s to offset %lld: %s\n", fname, (long long)offset, strerror(errno));
		exit(EXIT_FAILURE);
	}

	for (i = 0; !count || i < count; i++) {
		ch = getc(f);
		if (ch == EOF) {
			break;
		}
		if ((i % row_len) == 0) {
			if (i)
				putchar('\n');
			if (ptype != PREFIX_NONE) {
				long long po = (ptype == PREFIX_OFFSET ? offset : 0);
				printf("%*llx", offset_width, po + i);
			}
		}
		putchar(' ');
		putnibble(ch >> 4);
		putnibble(ch >> 0);
	}
	putchar('\n');
	if (count && i < count) {
		printf("EOF\n");
	}

	return 0;
}
