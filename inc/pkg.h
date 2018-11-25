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
	Membuf dirs;
	Membuf files;
	Membuf flags;
	Membuf mdeps;
	Membuf rdeps;
	off_t size;
	char name[PKG_NAMEMAX];
	char version[PKG_VERMAX];
	char license[PKG_LICMAX];
	char description[PKG_DESCMAX];
	char path[PKG_PATHMAX];
};

/* ar.c */
int uncomp(int, int);
int unarchive(int);

/* db.c */
Package * db_open(Package *, char *);

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
#define membuf_vstrcat(a, b, ...) membuf_vstrcat_((a), (b), __VA_ARGS__, NULL)
#define membuf_strinit(x, y) \
do {\
	membuf_strinit_((x), stackpool.p + stackpool.n, (y));\
	if ((stackpool.n += (x)->a) > stackpool.a)\
		errx(1, "buffer overflow");\
} while(0)

ssize_t membuf_dmemcat(Membuf *, void *, size_t);
ssize_t membuf_memcat(Membuf *, void *, size_t);
void    membuf_strinit_(Membuf *, char *, size_t);
ssize_t membuf_dstrcat(Membuf *, char *);
ssize_t membuf_strcat(Membuf *, char *);
ssize_t membuf_vstrcat_(Membuf *, char *, ...);

/* util.c */
unsigned filetosum(int, ssize_t *);
unsigned strtohash(char *);
long long strtobase(const char *, long long, long long, int);
mode_t strtomode(const char *, mode_t);
