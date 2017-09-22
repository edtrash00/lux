#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "pkg.h"

static int
pkg_add(const char *path)
{
	int rval = 0;
	Package *pkg;
	struct node *np;

	if (!(pkg = open_db(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	rval = mv(path, PKG_LDB);
	for (np = pkg->dirs; np; np = np->next)
		rval |= mv(np->data, PKG_DIR);
	for (np = pkg->files; np; np = np->next)
		rval |= mv(np->data, PKG_DIR);
	close_db(pkg);

	return rval;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s package ...\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int rval = 0;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++)
		rval |= pkg_add(*argv);

	return rval;
}
