#include <sys/types.h>

#include <stdio.h>

#include "arg.h"
#include "config.h"
#include "compat.h"

#define GETDB(x) \
((x) == LOCAL ? PKG_LDB : (x) == REMOTE ? PKG_RDB : (x) == NONE ? "." : NULL)

enum InfoFlags {
	AFLAG = 0x01, /* print about      */
	DFLAG = 0x02, /* list directories */
	FFLAG = 0x04, /* list files       */
	MFLAG = 0x08, /* list run deps    */
	PFLAG = 0x10, /* print prefix     */
	RFLAG = 0x20, /* list make deps   */
	PUTCH = 0x40  /* print space      */
};

enum RepoTypes {
	LOCAL,
	REMOTE,
	NONE
};

struct node {
	void *data;
	struct node *next;
};

typedef struct {
	struct node *dirs;
	struct node *files;
	struct node *flags;
	struct node *mdeps;
	struct node *rdeps;
	off_t size;
	off_t pkgsize;
	char *name;
	char *version;
	char *license;
	char *description;
} Package;

extern int curl_errno;

/* ar.c */
int unarchive(int);

/* db.c */
Package * db_open(const char *);
void db_close(Package *);

/* fgetline.c */
ssize_t fgetline(char *, size_t, FILE *);

/* fs.c */
int copy(const char *, const char *);
int move(const char *, const char *);
int remove(const char *);

/* net.c */
int netfd(char *, int, const char *);

/* node.c */
struct node * addelement(const void *);
void freenode(struct node *);
struct node * popnode(struct node **);
int pushnode(struct node **, struct node *);

/* util.c */
unsigned filetosum(int);
unsigned strtohash(char *);
size_t stoll(const char *, long long, long long, int);

/* src/util.c */
int add(Package *);
int del(Package *);
int fetch(Package *);
void info(Package *, int);
