#define _GNU_SOURCE 1

#include <features.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <openssl/ui_compat.h>

#define MAX_PASSWD 32
#define BUF_SIZE (64*1024)

typedef unsigned char byte;

char *progname;

byte stab[256] = {
 0x30, 0x0b, 0x90, 0x8d, 0xeb, 0xde, 0x14, 0xbc, 0x2b, 0xe2, 0xaa,
0x34, 0x61, 0xb2, 0xad, 0x6b,
 0x3a, 0xe9, 0xa6, 0xf4, 0x66, 0x4f, 0xc1, 0x38, 0xdc, 0x24, 0xc5,
0x60, 0x8e, 0xdb, 0x50, 0xac,
 0x49, 0xb8, 0x29, 0xe0, 0xb7, 0xed, 0x29, 0x80, 0x75, 0x43, 0x67,
0x1b, 0x6e, 0x20, 0x5f, 0x1f,
 0x5c, 0x26, 0x9c, 0x1a, 0xd9, 0x1e, 0xdc, 0x7b, 0x32, 0x20, 0xfc,
0xd1, 0x3a, 0xc4, 0x02, 0x25,
 0xd3, 0x0f, 0x83, 0x75, 0xdd, 0x3f, 0x3a, 0x56, 0x63, 0xd9, 0x5c,
0xc0, 0x2b, 0x9f, 0x8a, 0x4f,
 0x56, 0xe9, 0x29, 0x8b, 0x06, 0xd5, 0x1e, 0x2d, 0x17, 0xc5, 0xaa,
0x0e, 0x4e, 0xf6, 0x1a, 0x8b,
 0x0e, 0x3b, 0x19, 0x33, 0x99, 0x54, 0x2e, 0xd1, 0x6e, 0x01, 0xc5,
0x7a, 0xf1, 0x06, 0x69, 0x35,
 0x27, 0x6f, 0x79, 0x97, 0x8f, 0xf9, 0x35, 0x1c, 0xd4, 0x2c, 0xd5,
0x13, 0x86, 0x9a, 0x7d, 0x85,
 0xbe, 0x77, 0x75, 0x7a, 0x61, 0x25, 0xaf, 0x5a, 0x5c, 0x19, 0x1d,
0x24, 0xda, 0xf8, 0x19, 0x41,
 0xba, 0xc2, 0x84, 0x66, 0xe7, 0xd1, 0x09, 0x07, 0x2f, 0x6c, 0x80,
0x55, 0x29, 0x45, 0x24, 0x58,
 0x72, 0x6b, 0xd5, 0x8f, 0xa5, 0x16, 0xe8, 0x19, 0x7c, 0x10, 0xe7,
0xfa, 0xf2, 0xdc, 0xf0, 0x36,
 0x6d, 0xef, 0xd2, 0x90, 0x4c, 0x16, 0x52, 0x94, 0xc2, 0x96, 0x6e,
0x5e, 0x26, 0x96, 0x95, 0xe5,
 0x43, 0x03, 0x52, 0xc9, 0x1a, 0x18, 0x52, 0x21, 0x09, 0x08, 0x6e,
0x38, 0xcc, 0x89, 0xc8, 0x6a,
 0xec, 0xff, 0x18, 0x43, 0x73, 0xed, 0x67, 0xf3, 0x79, 0x08, 0xcd,
0xb5, 0x2c, 0x68, 0x5b, 0x1a,
 0x1a, 0x19, 0xe5, 0x2c, 0x91, 0x39, 0x02, 0x5d, 0xe2, 0x89, 0x69,
0x8c, 0x9e, 0xba, 0x33, 0xa2,
 0x56, 0xde, 0xc1, 0x58, 0x71, 0x92, 0x9c, 0xbf, 0xfa, 0x32, 0xd4,
0xd0, 0x48, 0x44, 0x79, 0x4d,
};

byte state;
byte etab[256] = {
 11, 104, 171, 74, 222, 235, 108, 150,
 216, 185, 52, 170, 178, 97, 13, 64,
 168, 58, 244, 106, 79, 102, 180, 89,
 65, 34, 225, 42, 118, 94, 87, 46,
 22, 198, 224, 41, 76, 132, 189, 113,
 67, 157, 197, 77, 8, 81, 9, 101,
 128, 92, 140, 176, 172, 119, 174, 25,
 202, 228, 71, 26, 196, 209, 183, 68,
 173, 29, 39, 179, 59, 51, 86, 61,
 253, 99, 192, 49, 181, 254, 93, 138,
 16, 70, 199, 117, 100, 124, 109, 10,
 15, 193, 125, 223, 137, 230, 145, 50,
 252, 126, 156, 220, 213, 153, 146, 215,
 30, 5, 206, 160, 152, 177, 203, 112,
 45, 245, 161, 163, 249, 40, 219, 38,
 184, 208, 130, 84, 169, 127, 133, 134,
 155, 190, 166, 143, 147, 90, 107, 175,
 33, 167, 237, 142, 234, 218, 211, 162,
 12, 227, 18, 62, 182, 191, 6, 43,
 85, 95, 188, 105, 247, 83, 115, 78,
 88, 151, 238, 54, 56, 165, 66, 232,
 233, 154, 250, 149, 55, 242, 123, 240,
 239, 241, 194, 210, 164, 2, 21, 139,
 20, 3, 148, 217, 32, 1, 229, 75,
 144, 214, 201, 37, 36, 47, 73, 195,
 207, 114, 44, 187, 205, 28, 122, 226,
 116, 236, 23, 60, 251, 121, 243, 80,
 200, 212, 159, 246, 48, 120, 135, 0,
 19, 4, 31, 129, 96, 186, 7, 63,
 204, 69, 248, 111, 82, 27, 136, 98,
 14, 110, 158, 131, 17, 35, 231, 221,
 57, 91, 255, 24, 141, 53, 103, 72,
};

byte dtab[256];

byte buf[BUF_SIZE];

#define IDX(_i_)  ((_i_) & 0xff)

static void
inittabs(void)
{
 int i;

 for (i = 0; i < 256; i++) {
   dtab[etab[i]] = i;
 }
}

static byte
enc(byte c)
{
 byte t, i = IDX(state + c), s;
 byte enc = etab[i];

 s = stab[i];
 t = etab[i];
 etab[i] = etab[s];
 etab[s] = t;
 dtab[etab[i]] = i;
 dtab[etab[s]] = s;

 stab[state] ^= stab[i];
 stab[i] ^= enc;
 state = stab[i];

 /*
 { static int n;
   fprintf(stderr, "%5d 0x%02x %d\n", n++, c, state);
 }
 */

 return enc;
}

static byte
dec(byte c)
{
 byte d = IDX(dtab[c] - state);
 enc(d);

 return d;
}

static void
reseed(const char *passkey)
{
 int i;
 byte c;
 const char *s;

 for (i = 0; i < 1000; i++) {
   for (s = passkey; (c = *s++) != 0; s++) {
     enc(c);
   }
 }
}

static void
enc_buf(byte *buf, int len)
{
 int i;

 for (i = 0; i < len; i++) {
   buf[i] = enc(buf[i]);
 }
}

static void
dec_buf(byte *buf, int len)
{
 int i;

 for (i = 0; i < len; i++) {
   buf[i] = dec(buf[i]);
 }
}

static void
errexit(char *fname, char *what)
{
 fprintf(stderr, "%s: cannot %s %s: %s\n",
         progname, what, fname, strerror(errno));
 exit(1);
}

static char *
mkstemplate(char *fname)
{
 char *s = malloc(strlen(fname) + 7);

 if (s) {
   strcpy(s, fname);
   strcat(s, "XXXXXX");
 }

 return s;
}

static char *
mktargetname(char *fname, int enc)
{
 int l = strlen(fname);
 static char ext[] = ".scr";
 int extlen = strlen(ext);
 char *s = malloc(l + extlen + 1);

 if (!s) {
   return NULL;
 }

 memcpy(s, fname, l+1);
 if (enc) {
   memcpy(s + l, ext, extlen + 1);
 } else if (l >= extlen && !strcmp(s + l - extlen, ext)) {
   s[l - extlen] = '\0';
 }

 return s;
}

static void
scram(void (*crypt)(byte *, int), char *fname, int inplace)
{
 size_t n;
 int fd, wfd;
 char *wfname;
 struct stat64 st;
 struct utimbuf ut;

 if (!fname) {
   fd = 0;
   fname = "stdin";
   inplace = 0;
 } else {
   fd = open64(fname, O_RDONLY);
   if (fd < 0) {
     errexit(fname, "open");
   }
 }

 if (!inplace) {
   wfd = 1;
   wfname = "stdout";
 } else {
   if (fstat64(fd, &st) < 0) {
     errexit(fname, "stat");
   }

   wfname = mkstemplate(fname);
   if (!wfname) {
     errexit("memory", "allocate");
   }
   wfd = mkstemp(wfname);
   if (wfd < 0) {
     errexit(wfname, "mkstemp");
   }
 }

 while ((n = read(fd, buf, sizeof(buf))) > 0) {
   crypt(buf, n);
   if (write(wfd, buf, n) != n) {
     if (wfd != 1) {
       remove(wfname);
     }
     errexit(wfname, "write");
   }
 }

 if (n < 0) {
   errexit(fname, "read");
 }

 close(fd);

 if (fsync(wfd) < 0) {
   if (wfd != 1) {
     remove(wfname);
   }
   errexit(wfname, "close");
 }

 if (inplace) {
   char *targetname;

   if (fchmod(wfd, st.st_mode) < 0) {
     fprintf(stderr, "%s: warning: could not chmod %s to mode %04o: %s",
             progname, wfname, st.st_mode, strerror(errno));
   }
   if (fchown(wfd, st.st_uid, st.st_gid) < 0) {
     fprintf(stderr, "%s: warning: could not chown %s to %u:%u: %s",
             progname, wfname, st.st_uid, st.st_gid, strerror(errno));
   }
   if (wfd != 1 && close(wfd) < 0) {
     if (wfd != 1) {
       remove(wfname);
     }
     errexit(wfname, "close");
   }

   ut.actime = st.st_atime;
   ut.modtime = st.st_mtime;
   if (utime(wfname, &ut) < 0) {
     fprintf(stderr, "%s: warning: could not utime %s to %ld:%ld: %s\n",
             progname, wfname, ut.actime, ut.modtime, strerror(errno));
   }

   targetname = mktargetname(fname, crypt == enc_buf);
   if (strcmp(fname, targetname)) {
     remove(targetname);
   }
   if (rename(wfname, targetname) < 0) {
     fprintf(stderr, "%s: warning: could not rename %s to %s: %s\n",
             progname, wfname, targetname, strerror(errno));
   }
   if (strcmp(fname, targetname)) {
     remove(fname);
   }
   free(targetname);
   free(wfname);
 }
}

int
main(int argc, char **argv)
{
 int ch, inplace = -1;
 void (* crypt)(byte *, int) = enc_buf;
 char passwd[MAX_PASSWD+1];

 progname = argv[0];
 if (strrchr(progname, '/') != NULL) {
   progname = strrchr(progname, '/') + 1;
 }

 while ((ch = getopt(argc, argv, ":huo")) != -1) {
   switch (ch) {
   case 'u':
     crypt = dec_buf;
     break;

   case 'o':
     inplace = 0;
     break;

   case 'h':
   case '?':
   default:
     fprintf(stderr, "Usage: %s [-uo] [file ...]\n", progname);
     exit(2);
   }
 }

 argc -= optind;
 argv += optind;

 if (progname[0] == 'u' || crypt == dec_buf) {
   crypt = dec_buf;
   des_read_pw_string(passwd, MAX_PASSWD, "Password: ", 0);
 } else {
   des_read_pw_string(passwd, MAX_PASSWD, "Password: ", 1);
 }

 inittabs();
 reseed(passwd);

 if (!argc) {
   scram(crypt, NULL, 0);
 } else while (argc--) {
   scram(crypt, *argv++, inplace >= 0 ? inplace : 1);
 }

 return 0;
}
