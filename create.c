#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	int opt;
	int rc, oflags = O_CREAT, omode = 0644;
	char *argv0, *name;

	argv0 = argv[0];
	if ((name = strrchr(argv0, '/')))
		argv0 = name + 1;

	while ((opt = getopt(argc, argv, "xm:")) != -1) {
		switch (opt) {
		case 'x':
			oflags |= O_EXCL;
			break;
		case 'm':
			omode = strtol(optarg, 0, optarg[0] == '0' ? 0 : 8);
			break;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Usage: %s [-m mode] file ...\n", argv0);
		exit(EXIT_FAILURE);
	}

	for ( ; optind < argc; optind++) {
		name = argv[optind];
		rc = open(name, oflags, omode);
		if (rc < 0) {
			fprintf(stderr, "%s: cannot create %s: %s\n", argv0, name, strerror(errno));
			exit(EXIT_FAILURE);
		}
		close(rc);
	}
	
	return 0;
}
