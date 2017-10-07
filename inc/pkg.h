#include <sys/types.h>

struct node {
	void *data;
	struct node *next;
};

typedef struct {
	struct node *dirs;
	struct node *files;
	struct node *flags;
	struct node *longdesc;
	struct node *mdeps;
	struct node *rdeps;
	off_t size;
	off_t pkgsize;
	char *name;
	char *longname;
	char *version;
	char *license;
	char *description;
} Package;

/* db.c */
Package * db_open(const char *);
void db_close(Package *);

/* download.c */
int download(char *, const char *, const char *);

/* fs.c */
int copy(const char *, const char *);
int move(const char *, const char *);
int remove(const char *);

/* node.c */
struct node * addelement(const void *);
void freenode(struct node *);
struct node * popnode(struct node **);
int pushnode(struct node **, struct node *);
