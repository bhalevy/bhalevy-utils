#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	FILE *f = stdin;
	int ch;
	int brace = 0;

	if (argc > 1)
		argv++;

	while ((ch = fgetc(f)) != EOF) {
		switch (ch) {
		case '{':
			brace++;
			if (brace > 1)
				continue;
			break;
		case '}':
			brace--;
			if (brace > 0)
				continue;
			break;
		default:
			if (brace)
				continue;
		}
		putchar(ch);
	}

	return 0;
}
