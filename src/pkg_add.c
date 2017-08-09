/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "fs.h"
#include "pkg.h"

static int
pkg_add(const char *path)
{
	int rval = 0;
	Package *pkg;
	struct node *current;

	if (!(pkg = open_db(path)))
		err(1, "open_db %s", path);

	for (current = pkg->dirs; current; current = current->next)
		rval |= copy_file(current->data, PKG_DIR);

	for (current = pkg->files; current; current = current->next)
		rval |= copy_file(current->data, PKG_DIR);

	rval |= copy_file(path, PKG_DB);

	close_db(pkg);

	return rval;
}

static int
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

	for (; *argv; argv++)
		rval |= pkg_add(*argv);

	return rval;
}
