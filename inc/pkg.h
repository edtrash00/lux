#include <sys/types.h>

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
#define PKG_LDB "/var/pkg/local"
#define PKG_RDB "/var/pkg/remote"
#define PKG_SRC "/var/pkg/source.list"
#define PKG_TMP "/var/pkg/cache"
#define PKG_FMT ".tar.gz"
#define PKG_SIG ".sig"

#define GETDB(x) \
((x) == LOCAL ? PKG_LDB : (x) == REMOTE ? PKG_RDB : (x) == NONE ? "." : NULL)

enum RType {
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

/* util.c */
unsigned long hash(char *);
long long stoll(const char *, long long, long long);

/* src */
int add_main(int, char **);
int del_main(int, char **);
int fetch_main(int, char **);
int info_main(int, char **);

int db_eopen(const char *, Package **);
