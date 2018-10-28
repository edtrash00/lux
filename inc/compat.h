#ifndef ALLPERMS
#define ALLPERMS    (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO)
#endif

#ifndef DEFFILEMODE
#define DEFFILEMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

#ifndef __progname
extern char *__progname;
#endif

#ifndef getprogname
#define getprogname( ) __progname
#endif

#ifndef setprogname
#define setprogname(x) __progname = x
#endif

#ifndef MIN
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#endif

#ifndef makedev
#define makedev(a, b) ((dev_t)(((a)<<8) | (b)))
#endif

#ifndef SHOW_NO_PB
#define PBINIT()
#define PBUPDATE(a, b) printf("\r%zd/%zd", a, b); fflush(stdout)
#define PBEND()        putchar('\n')
#else
#define PBINIT()
#define PBUPDATE(a, b)
#define PBEND()
#endif
