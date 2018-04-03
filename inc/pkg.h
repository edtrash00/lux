#include <sys/types.h>

#include <inttypes.h>
#include <stdio.h>

#include "arg.h"
#include "config.h"
#include "compat.h"

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

/* ar.c */
int uncomp(int, int);
int unarchive(int);

/* db.c */
Package * db_open(const char *);
void      db_close(Package *);

/* fgetline.c */
ssize_t fgetline(char *, size_t, FILE *);

/* fs.c */
int move(const char *, const char *);
int remove(const char *);

/* mode.c */
mode_t strtomode(const char *, mode_t);

/* net.c */
int netfd(char *, int, const char *);

/* node.c */
struct node * addelement(const void *);
void          freenode(struct node *);
struct node * popnode(struct node **);
int           pushnode(struct node **, struct node *);

/* util.c */
unsigned filetosum(int);
int mkdirp(char *, mode_t, mode_t);
unsigned strtohash(char *);
intmax_t strtobase(const char *, long long, long long, int);
