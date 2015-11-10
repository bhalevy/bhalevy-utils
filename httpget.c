/* this version from http://www-mae.engr.ucf.edu/~ambrose/httpget.c */

/* httpget -- use the http protocol to download files */
/* Copyright 1996, 1997, Steven Dick <ssd@nevets.oau.org>
 * Improvements 1997-2000 by Ambrose Feinstein <ambrose@mmae.engr.ucf.edu>
 */

/* Ideas:
 *   Need to escape characters in restp: [% ] + ctrl + highctrl
 *   This could support If-Modified-Since and POST
 *   Netscape cookies?
 *   specify a file containing additional headers to send in request
 *   specify an explicit range (start and end) to retrieve
 *     save range in correct place?
 *     save range at offset 0?
 *     resume code will probably need rewrite to handle ranges
 *   use getopt?
 *
 * Bugs:
 *   The output file is overwritten if the header indicates an error;
 *   this is not always undesirable.  Using -h without a filename is
 *   messy; this option is overloaded badly.  Resume breaks badly on
 *   1.0 servers.
 *
 * Bugs fixed:
 *  2-Apr-97
 *    If no / at end of hostname, incorrect URL was fetched.
 *  18-Jul-97 (AF)
 *    Several spelling errors corrected.
 *  18-Oct-97 (AF)
 *    Header was getting written to outfilename; fixed.  Header was
 *      not getting flushed/closed until entity was transferred; fixed.
 *  20-Dec-98 (AF)  version 1.81
 *    When no header was found, first line was discarded.  It was also
 *    difficult to get the header without changing it slightly, since
 *    it was parsed line by line.
 *  23-Dec-98 (AF)  version 1.82
 *    Header parsing is now case-insensitive.
 *  10-Apr-99 (AF)  version 1.84
 *    Fixed silly buglet where if the total transfer took less than one
 *    second, no status line was printed.
 *  19-Apr-99 (AF)  version 1.85
 *    Send \r\n at end of each line in request; some web servers seem
 *    to require this.
 *  26-Apr-99 (AF)  version 1.86
 *    Embarrassing stupidity in handling the output files fixed.
 *  21-Apr-01 (AF)  version 1.93
 *    Transfer-Encoding: chunked now understood, since 1.1 requires this.
 *    Why, I can't imagine, because it does nothing useful to me, so I
 *    should be able to turn it off.  I even send Connection: close and
 *    don't allow trailers!
 *
 * Features added:
 *  18-Jul-97 (AF)
 *    Time is now calculated more accurately than one second resolution.
 *    Option -m mode added.
 *    URL syntax is now more lenient--"http://" prefix is now optional
 *      since http protocol is assumed anyway.
 *  18-Oct-97 (AF)
 *    Option -r added.  Resumes download with -[oO] at the tail of
 *      the existing file.
 *  18-Oct-97 (ssd)
 *    readability and coding style improved ever so slightly
 *    several bugs fixed involving error handling and some missed test cases
 *  21-Oct-97 (AF)
 *    Now sends Host and User-Agent headers.
 *  2-Jan-98 (AF)
 *    Option -A added.
 *  12-Jan-98 (AF)  version 1.7
 *    Improved pacing of progress report; it was creating noticeable
 *      overhead in high volume transfers.
 *  20-Dec-98 (AF)  version 1.81
 *    Removed mess from some unimplemented options, wrote signal-based
 *    progress code, replaced ssd's i/o code, added #defines for various
 *    things, removed "ip as long" hostname parsing code (butchered things
 *    like "1st.net").
 *  9-Apr-99 (AF)  version 1.83
 *    Close sending half with shutdown after writing request.  Oops,
 *    nevermind, some web servers are too stupid for this to work.
 *  24-Jun-99 (AF)  version 1.9
 *    Option -a added.
 *  30-Jun-00 (AF)  version 1.91
 *    Headers tweaked to deal with a recalcitrant server.  Now claims
 *    to be an HTTP 1.1 client and Mozilla compatible, among other things.
 *  25-Aug-06 (BH)  version 2.00
 *    Improved options parsing.  Support for configuration file.
 */

#define VERSION "2.00"

/* disables -t option, results in slightly more efficient progress indicator */
/* (well, maybe) */
#define REPORT_VIA_ALARM

/* enables -a option */
#define BASIC_AUTH

/* buffer size for network and disk io */
#define IOBUFSIZE 16384

/* buffer size config file io */
#define CFGBUFSIZE 8192

/* maximum number of arguments in config file */
#define CFGMAXARGS 32

/* time between progress updates in microseconds */
#define REPORT_INTERVAL 200000

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#ifdef REPORT_VIA_ALARM
#include <signal.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>         /* struct sockaddr_in */
#include <netdb.h>              /* struct hostent */

const char *Progname="httpget";
const char *expected_status[]={"200","206"};

char *Proxy=0;
char *HeaderOutFileName=0, *OutFileName=0;
char *url=0;
#ifdef BASIC_AUTH
char *H_auth=0;
#endif
char H_add[4096]="";
int F_headonly=0;
int F_verbose=1;
int F_saveheader=0;
int F_sendheaders=1;
int F_progress=1;
int F_outurl=0;
int F_mode=-1;
int F_chunked=0;
off_t F_resume_point=0;


int sock;

int filesize=0, datasize=0;
struct timeval starttime, stoptime, lasttime={0,0};

void usage(){
  fprintf(stderr,
"httpget " VERSION "\n"
"Usage:  %s [options] URL\n"
"options:\n"
"  -o outputfile   save document to file instead of sending to stdout\n"
"  -O              like -o, but use the filename in the URL\n"
"  -q              quiet; don't print anything\n"
"  -s              print file transfer progress stats (sometimes default)\n"
#ifndef REPORT_VIA_ALARM
"  -t seconds      timeout and abort even if transfer is not completed\n"
#endif
"  -v              increase verboseness\n"
"  -H              only get header\n"
"  -h [outputfile] save header as well as body; in a separate file if given\n"
"  -p proxy        proxy host in URL format or host:port format\n"
"  -m mode         set file permissions of output\n"
"  -r              resume with -[oO] at current file endpoint\n"
"  -a header       also send header with request\n"
#ifdef BASIC_AUTH
"  -A user:pass    send \"basic\" authentication header\n"
#endif
,Progname);
  exit(1);
}

/* pass restp as 0 if you dont need it */
void parseurl(char*url, char**hostp, unsigned short *portp, char **restp) {
  char *s,*t;
  int i;

  /* take care of protocol type */
  if(strncmp(url,"http://",7)){
    if(F_verbose>=2)
      fprintf(stderr,"URL \"%s\" ambiguous.  Assuming protocol http.\n", url);
  }else
    url += 7;

  /* find end of host:port */
  s = strchr(url,'/');
  if(!s)
    s = url+strlen(url);
  /* is there a port? */
  t = strchr(url,':');
  if(t && t<s){
    *portp = atoi(t+1);
    i = t - url;
  }else{
    i = s - url;
    *portp = 80;
  }

  /* copy the hostname */
  *hostp = malloc(i+1);
  strncpy(*hostp,url,i);
  (*hostp)[i]=0;

  /* return the remainder */
  if(restp){
    if(s && *s){
      *restp = s;               /* this should look for things to escape */
    }else{
      *restp = "/";
    }
  }
}

#ifdef BASIC_AUTH
char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *base64_encode(unsigned char *s) {
  char *r;
  int i,l,m;
  l = strlen(s);
  m = (l+2) / 3;
  r = malloc(4*m + 1);
  for(i=0;i<m;i++){
    r[4*i] = base64_table[s[3*i] >> 2];
    if(3*i+1 < l){
      r[4*i+1] = base64_table[((s[3*i] & 3) << 4) | (s[3*i+1] >> 4)];
      if(3*i+2 < l){
        r[4*i+2] = base64_table[((s[3*i+1] & 15) << 2) | (s[3*i+2] >> 6)];
        r[4*i+3] = base64_table[s[3*i+2] & 63];
      }else{
        r[4*i+2] = base64_table[(s[3*i+1] & 15) << 2];
        r[4*i+3] = '=';
      }
    }else{
      r[4*i+1] = base64_table[(s[3*i] & 3) << 4];
      r[4*i+2] = '=';
      r[4*i+3] = '=';
    }
  }
  r[4*m] = 0;
  return r;
}
#endif

int xwrite(int fd, char *buf, int len) {
  int o,l,r;
  o=0;
  l=len;
  for(;;){
    r=write(fd, buf+o, l);
    if(r<0){
      if(errno==EAGAIN || errno==EINTR)
        continue;
      else
        return -1;
    }
    o+=r;
    l-=r;
    if(l)
      continue;
    return 0;
  }
}

int xread(int fd, char *buf, int len) {
  int o,l,r;
  o=0;
  l=len;
  for(;;){
    r=read(fd, buf+o, l);
    if(r<0){
      if(errno==EAGAIN || errno==EINTR)
        continue;
      else
        return -1;
    }
    if(r==0)
      return o;
    o+=r;
    l-=r;
    if(l)
      continue;
    return len;
  }
}

#ifdef REPORT_VIA_ALARM
int istime=1;

void catchalrm(int x) {
  istime=1;
}
#endif

void do_progress() {
  double transtime=0;
  struct timeval local_stoptime;
  char *sp;
  static char statline[100];

  lasttime = local_stoptime = stoptime;
  local_stoptime.tv_sec -= starttime.tv_sec;
  if((local_stoptime.tv_usec-=starttime.tv_usec) < 0){
    local_stoptime.tv_sec--;
    local_stoptime.tv_usec+=1000000;
  }
  sp = statline;
  if(F_resume_point)
    sp += sprintf(sp,"%lld+ ",(long long)F_resume_point);
  sp += sprintf(sp,"%6d",datasize);
  if(filesize)
    sp += sprintf(sp,"/%d",filesize);
  sp += sprintf(sp," bytes transferred in ");
  if(local_stoptime.tv_sec==0 && local_stoptime.tv_usec==0)
    sp += sprintf(sp,"0.0 seconds!\n");
  else{
    transtime = local_stoptime.tv_sec + ((double)local_stoptime.tv_usec/1000000);
    sp += sprintf(sp,"%0.2f seconds (%0.2f kB/sec).",transtime,((double)datasize/1024)/transtime);
  }
  while(sp<statline+78)
    *(sp++)=' ';
  *sp=0;
  fprintf(stderr,"\r%s\r",statline);
}

/*
 * Get an option's argument
 * If "required" is true and no valid argument is present
 * exit with usage() (never retrun NULL when arg is required)
 */
char *opt_arg(char *cp, char ***app, int required)
{
  /* arg can immediately follow option, e.g. -hhdr.txt */
  if (cp[1]) {
    return &cp[1];
  }

  (*app)++;
  cp = **app;
  if (cp == NULL) {
    if (required) {
      usage();
    }
    return NULL;
  } else if (cp[0] == '-' && cp[1] != '\0') {
    if (required) {
      usage();
    }
    return NULL;
  }

  return cp;
}

/* verify and parse command line options */
void parse_opts(char *argv[])
{
  char **ap, *cp;
  for(ap=argv; *ap; ap++){
    if(**ap!='-'){
      if (url) {
	usage();
      }
      url=*ap;
      continue;
    }
    for (cp=&ap[0][1]; *cp; cp++) {
      switch(*cp){
       case 'o':              /* output file */
	OutFileName = opt_arg(cp, &ap, 1);
	goto next;
       case 'O':              /* output filename from url */
	F_outurl++;
	break;
       case 'H':              /* header only */
	F_headonly++;
	F_saveheader++;
	break;
       case 'h':              /* save header */
	F_saveheader++;
	HeaderOutFileName = opt_arg(cp, &ap, 0);
	if(HeaderOutFileName) {
	  goto next;
	}
	break;
       case 'm':              /* set mode on output */
	F_mode=strtol(opt_arg(cp, &ap, 1), 0, 8);
	goto next;
       case 'r':              /* resume */
	F_resume_point=-1;
	break;
       case 'p':              /* proxy */
	Proxy = opt_arg(cp, &ap, 1);
	goto next;
       case 'q':              /* quiet */
	F_verbose = 0;
	F_progress = 0;
	break;
       case 's':              /* progress stats */
	F_progress++;
	break;
#ifndef REPORT_VIA_ALARM
       case 't':              /* countdown to self destruct */
	alarm(strtol(opt_arg(cp, &ap, 1),0,0));
	break;
#endif
       case 'v':              /* verbose level */
	F_verbose++;
	break;
       case 'a':              /* additional header */
	strcat(H_add,opt_arg(cp, &ap, 1));
	strcat(H_add,"\r\n");
	break;
#ifdef BASIC_AUTH
       case 'A':              /* authentication string */
	H_auth = base64_encode(opt_arg(cp, &ap, 1));
	break;
#endif
       default:
	fprintf(stderr,"Unknown option \"%s\"\n",*ap);
	usage();
	break;
      }
    }
   next:
    ;
  }
}

#define isnewline(c) ((c) == '\r' || (c) == '\n' || (c) == 0)
#define isspace(c) ((c) == ' ' || (c) == '\t')

enum {
  CFG_UNUSED,
  CFG_NEWLINE,
  CFG_COMMENT,
  CFG_SCAN,
  CFG_ARG
};

/* look up configuration options in ~/.httpgetrc or /etc/httpgetrc */
void config_opts(void)
{
  static char *path[] = { "~", "/etc", NULL };
  static char *pfx[] = { ".", "" };
  static char *sfx[] = { "rc", "rc" };
  int i, fd, state;
  size_t n;
  char *cp, c;
  char *args[CFGMAXARGS+1], **argv;
  char fname[256];
  char buf[CFGBUFSIZE + 1];

  cp = getenv("HOME");
  if (cp) {
    path[0] = cp;
  }

  argv = args;

  for (i = 0; path[i]; i++) {
    n = snprintf(fname, sizeof(fname), "%s/%s%s%s", path[i], pfx[i], Progname, sfx[i]);
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
      if (n >= sizeof(fname) - 1) {
	fprintf(stderr, "program name \"%s\" too long\n", Progname);
      }
      continue;
    }

    n = read(fd, buf, CFGBUFSIZE);
    if (n < 0) {
      fprintf(stderr, "Cannot read config file %s: %s\n",
	      fname, strerror(errno));
      close(fd);
      continue;
    }
    buf[n] = 0;

    cp = buf;
    state = CFG_NEWLINE;
    while (*cp) {
      c = *cp;
      switch (state) {
      case CFG_NEWLINE:
	if (c == '#') {
	  state = CFG_COMMENT;
	  cp++;
	} else if (!isnewline(c)) {
	  state = CFG_SCAN;
	}
	break;

      case CFG_COMMENT:
	if (isnewline(c)) {
	  state = CFG_NEWLINE;
	}
	cp++;
	break;

      case CFG_SCAN:
	if (isnewline(c)) {
	  state = CFG_NEWLINE;
	} else if (!isspace(c)) {
	  if (argv - args >= CFGMAXARGS) {
	    fprintf(stderr, "Too many args in config file %s\n",
		    fname);
	    goto out;
	  }
	  *argv++ = cp;
	  state = CFG_ARG;
	}
	cp++;
	break;

      case CFG_ARG:
	if (isspace(c)) {
	  *cp = 0;
	  state = CFG_SCAN;
	} else if (isnewline(c)) {
	  *cp = 0;
	  state = CFG_NEWLINE;
	}
	cp++;
	break;

      default:
	fprintf(stderr, "config_opts: Unknown state %d\n", state);
	goto out;
      }
    }

   out:
    close(fd);
    break;
  }

  if (argv > args) {
    *argv = NULL;
    parse_opts(args);
  }
}

int main(int argc,char *argv[]) {
  unsigned int a,b,c,d;
  struct sockaddr_in address;
  char *host,*geturl;
  char buf[IOBUFSIZE+16];
  unsigned short port;
  int i=0, j, done=0;
  int exitval=0, od;
#ifndef REPORT_VIA_ALARM
  struct timeval difftime;
#else
  struct itimerval alarmitv;
  struct sigaction act;
#endif
  FILE *out=0,*headerout=0;
  char *sp,*s;

  if(argv && argv[0]) {
    Progname = argv[0];
    s = strrchr(Progname, '/');
    if (s) {
      Progname = s + 1;
    }
  }

  config_opts();

  if(argv) {
    parse_opts(argv+1);
  }

  if(!url) usage();
  if(F_outurl){  /* generate output name from URL */
    char *s;
    s = strrchr(url,'/');
    if(s && s[1]) OutFileName = s+1;
    else{
      fprintf(stderr,"No '/' character found in url \"%s\"\n",url);
      usage();
    }
  }

  if(F_resume_point && !OutFileName){
    fprintf(stderr,"Resume on stdout ignored.\n");
    F_resume_point=0;
  }else if(F_resume_point){
    struct stat statbuf;
    if(stat(OutFileName,&statbuf))
      F_resume_point=0;
    else{
      F_resume_point=statbuf.st_size;
      if(F_verbose > 0)
        fprintf(stderr,"Resuming get at %lld bytes\n",(long long)F_resume_point);
    }
  }

  if(Proxy){
    if(!strncmp(Proxy,"http://",7))
      parseurl(Proxy,&host,&port,0);
    else{
      char *s;
      int i;
      s = strchr(Proxy,':');
      if(s){
        i = s-Proxy;
        host = malloc(i+1);
        strncpy(host,Proxy,i);
        host[i]=0;
        port = atoi(s+1);
      }else{
        host = strdup(Proxy);
        port = 80;
      }
    }
    geturl = url;
  }else
    parseurl(url,&host,&port,&geturl);
  if(F_verbose>2)
    fprintf(stderr,"Connecting to %s:%u to get %s\n",host,port,geturl);

/* build the address */
  memset(&address, 0, sizeof address);
  address.sin_family = AF_INET;
  address.sin_port = htons(port);

  if(4==sscanf(host,"%d.%d.%d.%d",&a,&b,&c,&d)){
    /* hostname given as ip? */
    long l = htonl((a<<24)+(b<<16)+(c<<8)+d);
    memcpy(&address.sin_addr,&l,sizeof l);
  }else{
    /* hostname */
    struct hostent *hp;
    if(F_verbose>1)
      fprintf(stderr,"Resolving hostname...");
    hp = gethostbyname(host);
    if(!hp){
      fprintf(stderr,"I can't locate host \"%s\".\n",host);
      exit(3);
    }
    memcpy(&address.sin_addr,hp->h_addr_list[0],hp->h_length);
    if(F_verbose>1)
      fprintf(stderr,"\r                      \r");
  }

/* open the socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock<0){
    perror("socket failed");
    exit(3);
  }
  if(F_verbose>1)
    fprintf(stderr,"Connecting...");
  if(connect(sock,(struct sockaddr*)&address,sizeof address)<0){
    perror("connect failed");
    exit(3);
  }
  if(F_verbose>1)
    fprintf(stderr,"\rWaiting...   ");

/* send the request header */
  *buf=0;
  if(F_headonly)
    strcat(buf,"HEAD ");
  else
    strcat(buf,"GET ");
  strcat(buf,geturl);
  strcat(buf," HTTP/1.1\r\n");
  strcat(buf,"Accept-Encoding: identity\r\n");
  strcat(buf,"Connection: close\r\n");
  strcat(buf,"Host: ");
  strcat(buf,host);
  if(port!=80)
    sprintf(buf+strlen(buf),":%d",port);
  strcat(buf,"\r\n");
  strcat(buf,"User-Agent: Mozilla/4.0 (compatible; httpget " VERSION ")\r\n");
  if(F_resume_point){
    char ns[80];
    sprintf(ns,"Range: bytes=%lld-\r\n",(long long)F_resume_point);
    strcat(buf,ns);
  }
#ifdef BASIC_AUTH
  if(H_auth){
    strcat(buf,"Authorization: Basic ");
    strcat(buf,H_auth);
    strcat(buf,"\r\n");
    /* can now free(H_auth) if we like */
  }
#endif
  strcat(buf,H_add);
  strcat(buf,"\r\n");
  xwrite(sock,buf,strlen(buf));
  /* done writing */
  /* should be able to do shutdown(sock,1) here, but some braindead servers
     stop sending if we do */

/* open output file or make arrangements... */
  if(OutFileName){
    od = open(OutFileName,O_WRONLY|O_CREAT,F_mode>=0?F_mode:0666);
    if(od<0){
      perror(OutFileName);
      exit(2);
    }
    lseek(od,F_resume_point,SEEK_SET);
    ftruncate(od,F_resume_point);
    if(F_mode>=0)
      fchmod(od,F_mode);
    out = fdopen(od,"w");
  }else{
    out = stdout;
  }

/* open header output file or make arrangements... */
  if(F_saveheader){
    if(!HeaderOutFileName){
      if(F_resume_point){
        fprintf(stderr,"Can't save header to output file when resuming.  Not saving header.\n");
        headerout=0;
      }else
        headerout = out;
    }else if(!strcmp(HeaderOutFileName,"-")){
      headerout = stdout;
    }else{
      headerout = fopen(HeaderOutFileName,"w");
      if(!headerout)
        perror(HeaderOutFileName);
    }
  }

  /* progress is only useful if stderr is visible but wont mix with out */
  if(!isatty(2) || (out==stdout && isatty(1)))
    F_progress--;

  gettimeofday(&starttime,0);
  lasttime = starttime;
  lasttime.tv_sec -= 3600;

/* wait for the header */
/** Example header:
 * HTTP/1.0 200 Document follows
 * MIME-Version: 1.0
 * Server: CERN/3.0pre6
 * Date: Friday, 23-Jun-95 17:26:59 GMT
 * Content-Type: text/html
 * Content-Length: 1672
 * Last-Modified: Saturday, 10-Jun-95 04:15:51 GMT
 */
  memset(buf,0,sizeof buf); /* paranoia; the tail always remains zero */
  j = xread(sock,buf,IOBUFSIZE);
  if(j<=0) {
    perror("network read error");
    exit(3);
  }
  buf[j] = 0;
  if(strncmp(buf,"HTTP/",5)){
    fprintf(stderr,"No header.\n");
    i=0;
  }else{
    /* check version and status */
    s=strchr(buf,' ');
    if(s){
      if(strncmp(s+1,expected_status[0!=F_resume_point],3)){
        exitval=4;
        if(F_verbose>0)
          fprintf(stderr,"Header return val \"%.3s\"\n",s+1);
      }
    }
    i=j;
    s=strstr(buf,"\n\n");
    sp=strstr(buf,"\r\n\r\n");
    if(!s || (s&&sp&&sp<s))
      s=sp;
    if(s){
      i=s-buf+2;
      if(s==sp)
        i+=2;
    }
    if(i>j)
      i=j;
    for(s=buf; s<buf+i; (s=strchr(s,'\n'))&&(s++)) {
      if(!strncasecmp(s, "Content-Length:", 15))
	filesize = atoi(s+15);
      if(!strncasecmp(s, "Transfer-Encoding: chunked", 26)) {
	F_chunked = 1;
	if(F_verbose>0)
	  fprintf(stderr,"server is using TE: chunked\n");
      }
    }
    if(headerout){
      fflush(headerout);
      xwrite(fileno(headerout),buf,i);
      if(headerout != out)
        fclose(headerout); /* all done with header file now */
    }
  }
  memmove(buf,buf+i,j-i);
  i=j-i;

  fflush(out);
  od=fileno(out);

#ifdef REPORT_VIA_ALARM
/* set up progress timer */
  if(F_progress){
    alarmitv.it_interval.tv_sec = alarmitv.it_value.tv_sec = 0;
    alarmitv.it_interval.tv_usec = 0;
    alarmitv.it_value.tv_usec = REPORT_INTERVAL;
    setitimer(ITIMER_REAL, &alarmitv, 0);
    memset(&act,0,sizeof act);
    act.sa_handler=catchalrm;
    sigaction(SIGALRM,&act,0);
  }
#endif

/* wait for the body */
  while(!done) {
    if(i) {
      if(F_chunked) {
        static int chunk_left = 0, chunk_count = 0;
        int ofs, wc, scan_failed;
        char *p;

        ofs = 0;
        while(ofs < i) {
          wc = chunk_left;
          if(wc > i-ofs)
            wc = i-ofs;
          if(wc)
            if(xwrite(od,buf+ofs,wc) < 0) {
              perror("file write error");
              exit(2);
            }
          datasize += wc;
          ofs += wc;
          chunk_left -= wc;
          if(ofs >= i)
            break;
          scan_failed = 0;
          if(chunk_count++) {
            if(strncmp(buf+ofs, "\r\n", 2))
              scan_failed = 1;
            ofs += 2;
          }
          p = strstr(buf+ofs, "\r\n");
          if(1!=sscanf(buf+ofs,"%x",&chunk_left))
            scan_failed = 1;
          if(!p)
            scan_failed = 1;
          if(scan_failed) {
            fprintf(stderr,"body not chunked as claimed\n");
            F_chunked = 0;
            break;
          }
          ofs = p+2-buf;
         if(chunk_left == 0)
           break;
        }
      }
      if(!F_chunked) {
        if(xwrite(od,buf,i) < 0) {
          perror("file write error");
          exit(2);
        }
        datasize+=i;
      }
    }

    /* print the statistics */
    if(
#ifdef REPORT_VIA_ALARM
      istime &&
#endif
      F_progress>0) {
#ifdef REPORT_VIA_ALARM
      istime=0;
      setitimer(ITIMER_REAL, &alarmitv, 0);
#endif
      gettimeofday(&stoptime,0);
#ifndef REPORT_VIA_ALARM
      difftime.tv_sec = stoptime.tv_sec - lasttime.tv_sec;
      if((difftime.tv_usec=stoptime.tv_usec-lasttime.tv_usec) < 0){
        difftime.tv_sec--;
        difftime.tv_usec+=1000000;
      }
      if((difftime.tv_sec*1000000+difftime.tv_usec)>=REPORT_INTERVAL) {
#endif
      do_progress();
#ifndef REPORT_VIA_ALARM
      }
#endif
    }

    i = xread(sock,buf,IOBUFSIZE);
    if(i > 0)
      buf[i] = 0;
    else
      done = 1;
  }

  if(F_progress > 0) {
    gettimeofday(&stoptime,0);
    do_progress();
  }

  if(i<0){
    perror("network read error");
    exit(3);
  }

  if(F_progress>0 || F_verbose>0)
    fprintf(stderr,"\n");

  if(out && out!=stdout)
    fclose(out);

  shutdown(sock,2);
  close(sock);
  exit(exitval);
}
