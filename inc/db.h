/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */

struct node {
	void *data;
	struct node *next;
};

typedef struct {
	char *name;
	char *version;
	char *license;
	char *description;
	struct node *files, *dirs;
	struct node *mdeps, *rdeps;
} Package;

/* db.c */
Package * open_db(const char *);
void close_db(Package *);

/* node.c */
struct node * addelement(const void *);
void freenode(struct node *);
struct node * popnode(struct node **);
void pushnode(struct node **, struct node *);
