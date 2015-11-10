#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void
usage(char *progname)
{
  fprintf(stderr, "Usage: %s [-u] [-f format] [-z timezone] seconds\n", progname);
  exit(1);
}

int
main(int argc, char **argv)
{
  int opt;
  char *progname = argv[0];
  char *fmt = "%a %b %e %T %Z %Y";
  int uflag = 0;
  time_t t;
  struct tm *tmp;
  char buf[65];

  while ((opt = getopt(argc, argv, ":uf:z:")) >= 0) {
    switch (opt) {
    case 'u':
      uflag++;
      break;
    case 'f':
      fmt = optarg;
      break;
    case 'z':
      setenv("TZ", optarg, 1);
      tzset();
      break;
    default:
      usage(progname);
    }
  }

  if (optind >= argc) {
    usage(progname);
  }

  while (optind < argc) {
    t = atol(argv[optind++]);
    tmp = uflag ? gmtime(&t) : localtime(&t);
    strftime(buf, sizeof(buf)-1, fmt, tmp);
    printf("%s\n", buf);
  }

  return 0;
}
