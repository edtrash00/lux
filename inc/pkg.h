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

typedef struct Node Node;

struct Node {
	char *data;
	Node *next;
};

typedef struct Package Package;

struct Package {
	Node *dirs;
	Node *files;
	Node *flags;
	Node *mdeps;
	Node *rdeps;
	off_t size;
	char *name;
	char *version;
	char *license;
	char *description;
	char *path;
};

/* ar.c */
int uncomp(int, int);
int unarchive(int);

/* db.c */
Package * db_open(const char *);
void      db_close(Package *);

/* fgetline.c */
ssize_t fgetline(char *, size_t, FILE *);

/* fs.c */
int copy(const char *, const char *);
int move(const char *, const char *);
int remove(const char *);
int mkdirp(char *, mode_t, mode_t);

/* net.c */
int netfd(char *, int, const char *);

/* node.c */
Node * addelement(const void *);
void   freenode(Node *);
Node * popnode(Node **);
int    pushnode(Node **, Node *);

/* str.c */
#define membuf_vstrcat(a, b, ...) membuf_vstrcat_((a), (b), __VA_ARGS__, NULL)
#define membuf_strinit(x, y) \
do {\
	if ((stackpool.n += (y)) > stackpool.a)\
		errx(1, "buffer overflow");\
	membuf_strinit_((x), stackpool.p+stackpool.n, (y));\
} while(0)

void    membuf_strinit_(Membuf *, char *, size_t);
ssize_t membuf_strcat(Membuf *, char *);
ssize_t membuf_vstrcat_(Membuf *, char *, ...);

/* util.c */
unsigned filetosum(int, ssize_t *);
unsigned strtohash(char *);
long long strtobase(const char *, long long, long long, int);
mode_t strtomode(const char *, mode_t);
