#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

enum {
	BUF_SIZE = 4 * 1024,
};

char *argv0;
char *fname;
char buf[BUF_SIZE];

void
errexit(char *what)
{
	fprintf(stderr, "%s: cannot %s %s: %s\n", argv0,
		what, fname, strerror(errno));
	exit(1);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s file [timeout_in_seconds]\n", argv0);
	exit(2);
}

int
main(int argc, char **argv)
{
	int fd;
	char *p;
	ssize_t count;
	int timeout = 60;

	argv0 = argv[0];
	if ((p = strrchr(argv0, '/')) != NULL)
		argv0 = p + 1;

	if (argc <= 1)
		usage();

	fname = argv[1];
	fd = open(fname, O_RDWR);
	if (fd < 0)
		errexit("open");

	if (argc > 2) {
		timeout = atoi(argv[2]);
		if (timeout < 0)
			usage();
	}

	count = read(fd, buf, sizeof(buf));
	if (count < 0)
		errexit("read");

	lseek(fd, 0, SEEK_SET);
	count = write(fd, buf, count);
	if (count < 0)
		errexit("write");

	if (fsync(fd) < 0)
		errexit("fsync");

	sleep(timeout);
	return 0;
}
