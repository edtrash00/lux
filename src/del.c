#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "pkg.h"

static int
rmfromsys(const char *path)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);

	if (remove(buf) < 0) {
		warn("remove %s", buf);
		return 1;
	}

	return 0;
}

static int
del(Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->files; np; np = np->next)
		rval |= rmfromsys(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= rmfromsys(np->data);

	return rval;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s del package ...\n", getprogname());
	exit(1);
}

int
del_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int rval = 0;
	Package *pkg;

	argc--, argv++; /* remove comand string */

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(LOCAL), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= del(pkg);
		db_close(pkg);

		if (remove(buf) < 0) {
			warn("remove %s", buf);
			rval = 1;
		}
	}

	return rval;
}
