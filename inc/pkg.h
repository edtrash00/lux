#include "arg.h"
#include "compat.h"

/* PKG_DIR: ROOT  PATH
 * PKG_LDB: LOCAL DATABASE
 * PKG_RDB: REPO  DATABASE
 * PKG_SRC: REPO  FILE
 * PKG_TMP: TMP   DIR
 * PKG_FMT: PKG   FORMAT
 * PKG_SIG: PKG   SIGNATURE
 */
#define PKG_DIR "/"
#define PKG_LDB "/var/pkg/ldb"
#define PKG_RDB "/var/pkg/rdb"
#define PKG_SRC "/var/pkg/source.list"
#define PKG_TMP "/tmp"
#define PKG_FMT ".tar.gz"
#define PKG_SIG ".sig"

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

/* download.c */
int download(char *, const char *, const char *);

/* fs.c */
int mv(const char *, const char *);
int wunlink(const char *);
int wrmdir(const char *);

/* node.c */
struct node * addelement(const void *);
void freenode(struct node *);
struct node * popnode(struct node **);
int pushnode(struct node **, struct node *);
