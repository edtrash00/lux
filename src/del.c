#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

static int
rmfromsys(const char *path)
{
	char buf[PATH_MAX];

	if (*PKG_DIR == '/')
		snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);
	else
		snprintf(buf, sizeof(buf), "%s/%s", PKG_DIR, path);

	if (remove(buf) < 0) {
		warn("remove %s", buf);
		return 1;
	}

	return 0;
}

static int
del(Package *pkg)
{
	char lp[PATH_MAX];
	int rval = 0;
	struct node *np;

	snprintf(lp, sizeof(lp), "%s/%s", PKG_LDB, pkg->name);

	for (np = pkg->files; np; np = np->next)
		rval |= rmfromsys(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= rmfromsys(np->data);

	if (remove(lp) < 0) {
		warn("remove %s", lp);
		return 1;
	}

	return rval;
}

int
del_main(int argc, char *argv[])
{
	int rval = 0;
	Package *pkg;

	argc--, argv++; /* remove comand string */

	for (; *argv; argc--, argv++) {
		if (db_eopen(LOCAL, *argv, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= del(pkg);
		db_close(pkg);
	}

	return rval;
}
