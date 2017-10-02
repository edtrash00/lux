#include <err.h>

#include "lux.h"

static int
eremove(const char *path)
{
	if (remove(path) < 0) {
		warn("remove %s", path);
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
		rval |= eremove(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= eremove(np->data);

	return rval;
}

int
del_main(int argc, char *argv[])
{
	int rval = 0;
	Package *pkg;

	for (; *argv; argc--, argv++) {
		if (db_eopen(*argv, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= del(pkg);
		db_close(pkg);
	}

	return rval;
}
