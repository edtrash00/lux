#include "arg.h"
#include "compat.h"
#include "pkg.h"

int add(const char *, Package *pkg);
int del(const char *, Package *pkg);
int fetch(const char *, Package *pkg);
int info(const char *, Package *pkg);

int warn_open_db(Package **, const char *);
int warn_mv(const char *, const char *);
int warn_rm(const char *);
