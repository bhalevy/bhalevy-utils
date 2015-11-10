/*
 * Author: David Howells <dhowells@redhat.com>
 * Source: http://www.spinics.net/lists/linux-cachefs/msg02415.html
 */
#define _XOPEN_SOURCE 500
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

static char *input;
static int fd;
static int lines, cols;
static size_t input_size;

static void cleanup(void)
{
	endwin();
}

static void sigint(int sig)
{
	endwin();
	signal(SIGINT, SIG_DFL);
	raise(SIGINT);
}

static __attribute__((noreturn))
void error(const char *str)
{
	int err = errno;

	endwin();
	fprintf(stderr, "%s: %s\n", str, strerror(err));
	exit(1);
}

/*
 * read from the file into the buffer and then display into the curses buffer,
 * truncating overlong lines
 */
static void display_file(void)
{
	ssize_t isize = 0, r;
	char *ip, *istop;
	int l, llen;

	do {
		r = pread(fd, input + isize, input_size - isize, isize);
		if (r == -1)
			error("read");
		isize += r;

	} while (r > 0 && isize < input_size);

	ip = input;
	for (l = 0; l < lines; l++) {
		if (isize <= 0)
			break;

		istop = memchr(ip, '\n', isize);
		llen = istop - ip;
		if (llen > cols)
			llen = cols;
		if (llen > 0)
			mvaddnstr(l, 0, ip, llen);
		if (llen < cols)
			clrtoeol();
		istop++;
		isize -= istop - ip;
		ip = istop;
	}

	if (l < lines)
		clrtobot();
}

/*
 *
 */
int main(int argc, char **argv)
{
	int ch, paused = 0;

	if (argc != 2)
		exit(2);

	if (!isatty(1)) {
		fprintf(stderr, "stdout not a TTY\n");
		exit(3);
	}

	fd = open(argv[1], O_RDONLY, 0);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	signal(SIGINT, sigint);
	atexit(cleanup);

	initscr();
	cbreak();
	noecho();

	for (;;) {
		if (!input) {
			lines = LINES;
			cols = COLS;
			input_size = lines * (cols + 1) * 4;
			input = calloc(1, input_size + 1);
			if (!input)
				error("calloc");
		}

		if (!paused) {
			display_file();
			move(lines - 1, cols - 1);
			refresh();
		}

		if (halfdelay(1) == OK) {
			ch = getch();
			if (ch == 'q')
				exit(0);
			if (ch == ' ') {
				paused = !paused;
				if (paused) {
					mvaddch(0, cols - 8, '[');
					standout();
					addnstr("PAUSED", 6);
					standend();
					addch(']');
				}
			}
			if (ch == KEY_RESIZE) {
				free(input);
				input = NULL;
			}
		}
	}
}
