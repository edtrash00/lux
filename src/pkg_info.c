/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "pkg.h"

static void
pkg_info(const char *path)
{
	Package *pkg;
	struct node *current;

	if (!(pkg = open_db(path)))
		err(1, "open_db %s", path);

	printf("Name: %s\n"
	       "Version: %s\n"
	       "License: %s\n"
	       "Description: %s\n",
	       pkg->name, pkg->version, pkg->license, pkg->description);

	puts("\nDependencies:");
	for (current = pkg->rdeps; current; current = current->next)
		printf("R: %s\n", (char *)current->data);

	for (current = pkg->mdeps; current; current = current->next)
		printf("M: %s\n", (char *)current->data);

	puts("\nFiles:");
	for (current = pkg->dirs; current; current = current->next)
		printf("D: %s\n", (char *)current->data);

	for (current = pkg->files; current; current = current->next)
		printf("F: %s\n", (char *)current->data);

	close_db(pkg);
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
	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	for (; *argv; argv++)
		pkg_info(*argv);

	return 0;
}
