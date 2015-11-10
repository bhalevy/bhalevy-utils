#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

int uflag = 0;

void
usage(char *argv0)
{
  fprintf(stderr, "Usage: %s [-u] file ...\n", argv0);
  exit(EXIT_FAILURE);
}

void
ptime(char *desc, time_t sec, long nsec)
{
  struct tm *t;
  char *s;
  unsigned long gmtoff;

  t = uflag ? gmtime(&sec) : localtime(&sec);
  s = t->tm_gmtoff >= 0 ? "" : "-";
  gmtoff = t->tm_gmtoff >= 0 ? t->tm_gmtoff : - t->tm_gmtoff;

  printf("%s: %4d-%02d-%02d %02d:%02d:%02d.%09ld %s%02ld%02ld %s\n",
    desc, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec, nsec,
    s, gmtoff / 3600, (gmtoff % 3600) / 60, t->tm_zone);
}

int
main(int argc, char **argv)
{
  int i;
  char ch;
  char *fname;
  char *ftype = "Unknown file type";
  char mchar = '-';
  struct stat s;
  struct passwd *pw;
  struct group *gr;

  if (argc <= 1) {
    usage(argv[0]);
  }

  while ((ch = getopt(argc, argv, "uh")) != -1)
    switch (ch) {
    case 'u':
      uflag = 1;
      break;
    case '?':
    default:
      usage(argv[0]);
  }
  argc -= optind;
  argv += optind;

  for (i = 0; i < argc; i++) {
    fname = argv[i];
    if (stat(fname, &s) < 0) {
      if (!strncmp(fname, "-h", 2)) {
        usage(argv[0]);
      }
      fprintf(stderr, "%s: %s: %s\n", argv[0], fname, strerror(errno));
      exit(EXIT_FAILURE);
    }

    switch (s.st_mode & S_IFMT) {
    case S_IFIFO: mchar = 'p'; ftype = "Named Pipe (FIFO)"; break;
    case S_IFCHR: mchar = 'c'; ftype = "Character Special"; break;
    case S_IFDIR: mchar = 'd'; ftype = "Directory"; break;
    case S_IFBLK: mchar = 'b'; ftype = "Block Special"; break;
    case S_IFREG: mchar = '-'; ftype = "Regular File"; break;
    case S_IFLNK: mchar = 'l'; ftype = "Symbolic Link"; break;
    case S_IFSOCK: mchar = 's'; ftype = "Socket"; break;
#ifdef FREEBSD
    case S_IFWHT: mchar = '-'; ftype = "Whiteout"; break;
#endif
    }

    pw = getpwuid(s.st_uid);
    gr = getgrgid(s.st_gid);

    printf("  File: `%s'\n", fname);
    printf("  Size: %-15llu Blocks: %-10llu IO Block: %-6u %s\n",
      (unsigned long long)s.st_size, (long long unsigned)s.st_blocks,
      (unsigned)s.st_blksize, ftype);
    printf("Device: %-7x Inode: %-11u Links %u\n",
      (unsigned)s.st_dev, (unsigned)s.st_ino, (unsigned)s.st_nlink);
    printf("Access: (%04o/%c%c%c%c%c%c%c%c%c%c)  Uid: (%5u/%8s)   Gid: (%5u/%8s)\n",
      s.st_mode & ALLPERMS, mchar,
      s.st_mode & S_IRUSR ? 'r' : '-',
      s.st_mode & S_IWUSR ? 'w' : '-',
      s.st_mode & S_IXUSR ?
        (s.st_mode & S_ISUID ? 's' : 'x') :
        (s.st_mode & S_ISUID ? 'S' : '-'),
      s.st_mode & S_IRGRP ? 'r' : '-',
      s.st_mode & S_IWGRP ? 'w' : '-',
      s.st_mode & S_IXGRP ?
        (s.st_mode & S_ISGID ? 's' : 'x') :
        (s.st_mode & S_ISGID ? 'S' : '-'),
      s.st_mode & S_IROTH ? 'r' : '-',
      s.st_mode & S_IWOTH ? 'w' : '-',
      s.st_mode & S_IXOTH ?
        (s.st_mode & S_ISVTX ? 't' : 'x') :
        (s.st_mode & S_ISVTX ? 'T' : '-'),
      s.st_uid, pw ? pw->pw_name : "",
      s.st_gid, gr ? gr->gr_name : "");
#ifdef FREEBSD
    ptime("Access", s.st_atimespec.tv_sec, s.st_atimespec.tv_nsec);
    ptime("Modify", s.st_mtimespec.tv_sec, s.st_mtimespec.tv_nsec);
    ptime("Change", s.st_ctimespec.tv_sec, s.st_ctimespec.tv_nsec);
#else
    ptime("Access", s.st_atime, 0);
    ptime("Modify", s.st_mtime, 0);
    ptime("Change", s.st_ctime, 0);
#endif
    printf("\n");
  }

  return EXIT_SUCCESS;
}
