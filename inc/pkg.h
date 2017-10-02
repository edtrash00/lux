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
