#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static long
kmgt(char *s)
{
	long n = strtol(s, &s, 0);

	switch (*s) {
	case 'T': case 't':
		n <<= 10;
	case 'G': case 'g':
		n <<= 10;
	case 'M': case 'm':
		n <<= 10;
	case 'K': case 'k':
		n <<= 10;
	}

	return n;
}

int
main(int argc, char **argv)
{
	int fd, error;
	char *fname;
	off_t toffset, woffset;
	size_t count;
	unsigned char *buf;

	if (argc != 5) {
		fprintf(stderr,
			"Usage: %s filename truncate_offset "
			"write_offset count\n",
			argv[0]);
		exit(1);
	}

	fname = argv[1];
	toffset = kmgt(argv[2]);
	woffset = kmgt(argv[3]);
	count = kmgt(argv[4]);

	buf = malloc(count);
	if (!buf) {
		fprintf(stderr, "%s: malloc %Zd: %s\n",
			argv[0], count, strerror(errno));
		exit(1);
	}
	memset(buf, 0xff, count);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "%s: %s: %s:\n",
			argv[0], fname, strerror(errno));
		exit(1);
	}

	error = ftruncate(fd, toffset);
	if (error < 0) {
		fprintf(stderr, "%s: ftruncate %ld: %s:\n",
			argv[0], toffset, strerror(errno));
		exit(1);
	}

	if (lseek(fd, woffset, SEEK_SET) != woffset) {
		fprintf(stderr, "%s: lseek %ld: %s:\n",
			argv[0], woffset, strerror(errno));
		exit(1);
	}

	if ((size_t)write(fd, buf, count) != count) {
		fprintf(stderr, "%s: write %Zd: %s:\n",
			argv[0], count, strerror(errno));
		exit(1);
	}

	if (close(fd) < 0) {
		fprintf(stderr, "%s: close: %s:\n",
			argv[0], strerror(errno));
		exit(1);
	}

	return 0;
}
