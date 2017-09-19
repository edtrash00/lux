/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include "arg.h"
#include "compat.h"

#define PKG_DIR "/"
#define PKG_DB "/var/pkg/db"

struct node {
	void *data;
	struct node *next;
};

typedef struct {
	char *name;
	char *version;
	char *license;
	char *description;
	struct node *files;
	struct node *dirs;
	struct node *mdeps;
	struct node *rdeps;
} Package;


/* db.c */
Package * open_db(const char *);
void close_db(Package *);

/* fs.c */
int mv(const char *, const char *);
int wunlink(const char *);
int wrmdir(const char *);

/* node.c */
struct node * addelement(const void *);
void freenode(struct node *);
struct node * popnode(struct node **);
int pushnode(struct node **, struct node *);
