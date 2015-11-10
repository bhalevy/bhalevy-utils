#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/user.h>

char *argv0;

void
usage(void)
{
	fprintf(stderr, "Usage: %s size[kmg]\n", argv0); 
	exit(1);
}

size_t
kmg(char *s)
{
	unsigned long long n = strtoull(s, &s, 0);

	if (!s)
		return n;
	switch (*s) {
	case 'g': case 'G': n *= 1024;
	case 'm': case 'M': n *= 1024;
	case 'k': case 'K': n *= 1024;
		break;
	default:
		usage();
	}
	return n;
}

int
main(int argc, char **argv)
{
	char *p;
	size_t sz = 0;
	int n;

	argv0 = argv[0];
	if ((p = strrchr(argv[0], '/')))
		argv0 = p + 1;

	if (argc > 1)
		sz = kmg(argv[1]);

	if (!sz)
		usage();

	p = malloc(sz);
	if (!p) {
		fprintf(stderr, "%s: malloc %Zu failed: %s\n",
		        argv0, sz, strerror(errno));
		exit(1);
	}

	for (n = PAGE_SIZE; sz; p += n, sz -= n) {
		*(int*)p = 0;
		if (n > sz)
			n = sz;
	}

	return 0;
}
