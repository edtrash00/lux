#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

static int
emove(const char *src, const char *dest)
{
	char buf[PATH_MAX];

	if (*dest == '/')
		snprintf(buf, sizeof(buf), "%s%s", dest, src);
	else
		snprintf(buf, sizeof(buf), "%s/%s", dest, src);

	if (move(src, buf) < 0) {
		warn("move %s -> %s", src, buf);
		return 1;
	}

	return 0;
}

static int
add(Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->dirs; np; np = np->next)
		rval |= emove(np->data, PKG_DIR);
	for (np = pkg->files; np; np = np->next)
		rval |= emove(np->data, PKG_DIR);

	return rval;
}

int
add_main(int argc, char *argv[])
{
	int rval = 0;
	Package *pkg;

	for (; *argv; argc--, argv++) {
		if (db_eopen(*argv, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= add(pkg);
		db_close(pkg);
	}

	return rval;
}
