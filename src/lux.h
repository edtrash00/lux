#include "arg.h"
#include "compat.h"
#include "pkg.h"

int add(Package *pkg, const char *);
int del(Package *pkg, const char *);
int fetch(Package *pkg);
int info(Package *pkg);

int warn_open_db(Package **, const char *);
int warn_mv(const char *, const char *);
int warn_rm(const char *);
