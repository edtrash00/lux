#ifndef __progname
extern char *__progname;
#endif

#ifndef getprogname
#define getprogname( ) __progname
#endif

#ifndef setprogname
#define setprogname(x) __progname = x
#endif
