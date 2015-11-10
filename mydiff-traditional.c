#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define DEF_NLINES (8 * 1024)

char *argv0;
int debug = 0;
int nsync = 3;
int maxfuzz = 250;
int maxline = 1024 + 1;

typedef struct {
	char *fname;
	char **lines;
	char *linediff;
	int nalloclines;
	int nlines;
} file_t;

file_t files[2];
char *linebuf;
int hunkstart[2];
int hunkstop[2];
int hunktag = -1;

static void *
myalloc(size_t size)
{
	void *p = malloc(size);

	if (p) {
		memset(p, 0, size);
		return p;
	}

	fprintf(stderr, "%s: could not allocate %Zd bytes: %s\n",
		argv0, size, strerror(errno));
	exit(1);
}

static void *
myrealloc(void *s, size_t size)
{
	void *p = realloc(s, size);

	if (p)
		return p;

	fprintf(stderr, "%s: could not reallocate %Zd bytes: %s\n",
		argv0, size, strerror(errno));
	exit(1);
}

static char *
mystrdup(char *s)
{
	int len = strlen(s) + 1;
	char *p = myalloc(len);

	memcpy(p, s, len);
	return p;
}

static FILE *
myopen(char *fname)
{
	FILE *f = fopen(fname, "r");

	if (f)
		return f;

	fprintf(stderr, "%s: open %s: %s\n",
		argv0, fname, strerror(errno));
	exit(1);
}

static void
readfile(file_t *fp, char *fname)
{
	FILE *f = myopen(fname);
	char *s;

	fp->fname = fname;
	fp->lines = myalloc(DEF_NLINES * sizeof(*fp->lines));
	fp->linediff = myalloc(DEF_NLINES * sizeof(*fp->linediff));
	fp->nalloclines = DEF_NLINES;
	fp->nlines = 0;
	for (;;) {
		s = fgets(linebuf, maxline, f);
		if (!s)
			break;
		if (fp->nlines >= fp->nalloclines) {
			fp->lines = myrealloc(fp->lines,
				2 * fp->nalloclines * sizeof(*fp->lines));
			fp->linediff = myrealloc(fp->linediff,
				2 * fp->nalloclines * sizeof(*fp->linediff));
			fp->nalloclines *= 2;
		}
		fp->lines[fp->nlines++] = mystrdup(linebuf);
		if (debug > 1)
			fprintf(stderr, "%s: %s %d\n", __FUNCTION__, fp->fname, fp->nlines);
	}
}

static inline char *
fline(int id, int line)
{
	return files[id].lines[line];
}

static inline int
fdiff(int id, int line)
{
	return files[id].linediff[line];
}

static int
fnext(int id, int line)
{
	if (line < 0)
		return -1;
	return ++line < files[id].nlines ? line : -1;
}

static int
fmaxline(int id, int line)
{
	return line >= 0 && line < files[id].nlines ? line : files[id].nlines - 1;
}

static int
cmpline(int *pos)
{
	if (debug > 1)
		fprintf(stderr, "%s: %d %d\n", __FUNCTION__, pos[0], pos[1]);

	if (pos[0] >= files[0].nlines)
		pos[0] = -1;
	if (pos[1] >= files[1].nlines)
		pos[1] = -1;

	if (pos[0] < 0 && pos[1] < 0)
		return -1;

	if (pos[0] < 0 || pos[1] < 0 ||
	    strcmp(fline(0, pos[0]), fline(1, pos[1])))
		return 1;

	return 0;
}

static void
finddiff(int pos[2])
{
	int res;

	for (;;) {
		res = cmpline(pos);
		if (res)
			break;
		pos[0] = fnext(0, pos[0]);
		pos[1] = fnext(1, pos[1]);
	}
}

static int
findline(char *s, int fid, int l)
{
	if (l < 0)
		return -1;
	for (; l < files[fid].nlines; l++)
		if (!strcmp(s, fline(fid, l)))
			return l;
	return -1;
}

static void
printline(int id, int line)
{
	printf("%c%s",
		files[id].linediff[line] == 0 ? ' ' :
			files[id].linediff[line] < 0 ? '-' : '+',
		fline(id, line));
}

void
printhunk(void)
{
	int i, n;

	n = fmaxline(0, hunkstart[0]);
	i = nsync;
	if (i > n)
		i = n;

	if (debug)
		printf("&& %d,%d %d,%d (%d) &&\n",
			hunkstart[0], hunkstop[0],
			hunkstart[1], hunkstop[1],
			i);
	hunkstart[0] -= i;
	hunkstart[1] -= i;
	printf("@@ -%d,%d +%d,%d @@\n",
		hunkstart[0] + 1, hunkstop[0] - hunkstart[0] + 1,
		hunkstart[1] + 1, hunkstop[1] - hunkstart[1] + 1);

	do {
		if (hunkstart[0] >= 0 && hunkstart[0] <= hunkstop[0] &&
		    fdiff(0, hunkstart[0])) {
			printline(0, hunkstart[0]);
			hunkstart[0] = fnext(0, hunkstart[0]);
		} else {
			if (hunkstart[0] >= 0 && hunkstart[0] <= hunkstop[0] &&
			    !fdiff(1, hunkstart[1]))
				hunkstart[0] = fnext(0, hunkstart[0]);
			printline(1, hunkstart[1]);
			hunkstart[1] = fnext(1, hunkstart[1]);
		}
	} while ((hunkstart[0] >= 0 && hunkstart[0] < hunkstop[0]) ||
		 (hunkstart[1] >= 0 && hunkstart[1] < hunkstop[1]));
}

int
main(int argc, char **argv)
{
	int pos[2];
	int i, j, back;
	char *p;

	argv0 = argv[0];
	p = strrchr(argv0, '/');
	if (p)
		argv0 = p + 1;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s file1 file2\n", argv0);
		return 1;
	}

	linebuf = myalloc(maxline);

	readfile(&files[0], argv[1]);
	readfile(&files[1], argv[2]);

	printf("diff -u %s %s\n", files[0].fname, files[1].fname);
	printf("--- %s\n", files[0].fname);
	printf("+++ %s\n", files[1].fname);

	pos[0] = pos[1] = 0;
	for (;;) {
		int ns;

		finddiff(pos);
		if (pos[0] < 0 && pos[1] < 0)
			break;

		memcpy(hunkstart, pos, sizeof(pos));
		hunkstop[0] = fmaxline(0, -1);
		hunkstop[1] = fmaxline(1, -1);
		for (ns = 0; pos[0] >= 0 && ns < nsync * 2; ) {
			i = findline(fline(0, pos[0]), 1, pos[1]);

			if (i < 0) {
//notfound:
				files[0].linediff[pos[0]] = -1;
				/* backpatch */
				if (ns > 0 && ns <= 2) {
					for (j = 0; j < ns; j++)
						files[0].linediff[hunkstop[0] - j] = -1;
					for (j = back; j < hunkstop[1]; j++)
						files[1].linediff[j] = 0;
					pos[1] = back;
				}
				ns = 0;
				hunkstop[0] = fmaxline(0, -1);
				hunkstop[1] = fmaxline(1, -1);
			} else {
				if (i > pos[1]) {
/*
					if (i - pos[1] > maxfuzz) {
						i = -1;
						goto notfound;
					}
*/
					for (j = pos[1]; j < i; j++)
						files[1].linediff[j] = 1;
					ns = 0;
				}
				hunkstop[0] = pos[0];
				hunkstop[1] = i;
				if (!ns++)
					back = pos[1];
				pos[1] = fnext(1, i);
			}
			pos[0] = fnext(0, pos[0]);
		}

		if (ns >= nsync * 2) {
			hunkstop[0] -= nsync;
			hunkstop[1] -= nsync;
		}

		printhunk();
		pos[0] = fnext(0, hunkstop[0]);
		pos[1] = fnext(1, hunkstop[1]);
	}

	return 0;
}
