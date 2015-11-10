#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char *argv0;

void
usage(void)
{
	fprintf(stderr, "Usage %s src ... dest\n", argv0);
	exit(EXIT_FAILURE);
}

void
syserr(const char *op, const char *path)
{
	fprintf(stderr, "%s: %s %s: %s\n", argv0, op, path, strerror(errno));
	exit(EXIT_FAILURE);
}

static const char *
basename(const char *path)
{
	char *p = strrchr(path, '/');
	return p ? p + 1 : path;
}

int
main(int argc, char **argv)
{
	int dest_is_dir = 0;
	int i, rc = 0;
	char *dest;
	struct stat st;

	argv0 = basename(argv[0]);
	if (argc < 3)
		usage();

	dest = argv[--argc];

	rc = stat(dest, &st);
	if (!rc) {
		dest_is_dir = (st.st_mode & __S_IFDIR) != 0;
		if (!dest_is_dir) {
			fprintf(stderr, "%s: %s: file exists\n", argv0, dest);
			usage();
		}
	}
	if (!dest_is_dir && argc > 2) {
		fprintf(stderr, "%s: %s: not a directory\n", argv0, dest);
		usage();
	}

	if (dest_is_dir) {
		int n, len = 0;
		
		for (i = 1; i < argc; i++) {
			n = strlen(basename(argv[i]));
			if (n > len)
				len = n;
		}
		dest = malloc(strlen(argv[argc]) + len + 2);
	}
	for (i = 1, argc; i < argc; i++) {
		if (dest_is_dir)
			sprintf(dest, "%s/%s", argv[argc], basename(argv[i]));
		if ((rc = rename(argv[i], dest)) != 0) {
			fprintf(stderr, "%s: rename %s -> %s: %s\n", argv0,
				argv[i], dest, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
