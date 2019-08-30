#include <sys/types.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include "arg.h"
#include "config.h"
#include "compat.h"

typedef struct Membuf Membuf;

struct Membuf {
	size_t  a;
	size_t  n;
	char   *p;
};

typedef struct Package Package;

struct Package {
	FILE  *fp;
	Membuf description;
	Membuf license;
	Membuf mdeps;
	Membuf name;
	Membuf path;
	Membuf rdeps;
	Membuf version;
	off_t  size;
};

/* alloc.c */
void * alloc(size_t);
void   alloc_free(void *, size_t);
void * alloc_re(void *, size_t, size_t);
void * ialloc(void);
void * ialloc_re(void *, size_t, size_t);
void * salloc(size_t);
void * scalloc(size_t, size_t);
void   sfree(void *);
void * srealloc(void *, size_t);
void   sfreeall(void);

/* ar.c */
int uncomp(int, int);
int unarchive(int);

/* db.c */
Package * db_open(Package *, char *);
char    * db_walkfile(Package *);

/* fgetline.c */
ssize_t fgetline(char *, size_t, FILE *);

/* fs.c */
int copy(const char *, const char *);
int move(const char *, const char *);
int remove(const char *);
int mkdirp(char *, mode_t, mode_t);

/* net.c */
int netfd(char *, int, const char *);

/* str.c */
void    membuf_free(Membuf *);
#define membuf_vstrcat(a, b, ...) membuf_vstrcat_((a), (b), __VA_ARGS__, NULL)
void    membuf_strinit(Membuf *);
ssize_t membuf_strcat(Membuf *, char *);
ssize_t membuf_vstrcat_(Membuf *, char *, ...);

/* util.c */
int chksum(Package *, size_t, unsigned);
unsigned filetosum(int, size_t *);
unsigned strtohash(char *);
long long strtobase(const char *, long long, long long, int);
mode_t strtomode(const char *, mode_t);
char * dircomp(const char *);
int s_chown(const char *, uid_t, gid_t);
int s_lchown(const char *, uid_t, gid_t);
