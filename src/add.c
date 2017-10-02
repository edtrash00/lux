#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

static int
mvtosys(const char *path)
{
	char buf[PATH_MAX];

	if (*PKG_DIR == '/')
		snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);
	else
		snprintf(buf, sizeof(buf), "%s/%s", PKG_DIR, path);

	if (move(path, buf) < 0) {
		warn("move %s -> %s", path, buf);
		return 1;
	}

	return 0;
}

static int
add(Package *pkg)
{
	char lp[PATH_MAX], rp[PATH_MAX];
	int rval = 0;
	struct node *np;

	snprintf(lp, sizeof(lp), "%s/%s", PKG_LDB, pkg->name);
	snprintf(rp, sizeof(rp), "%s/%s", PKG_RDB, pkg->name);

	for (np = pkg->dirs; np; np = np->next)
		rval |= mvtosys(np->data);
	for (np = pkg->files; np; np = np->next)
		rval |= mvtosys(np->data);

	if (copy(rp, lp) < 0) {
		warn("copy %s -> %s", rp, lp);
		return 1;
	}

	return rval;
}

int
add_main(int argc, char *argv[])
{
	int rval = 0;
	Package *pkg;

	argc--, argv++; /* remove comand string */

	for (; *argv; argc--, argv++) {
		if (db_eopen(REMOTE, *argv, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= add(pkg);
		db_close(pkg);
	}

	return rval;
}
