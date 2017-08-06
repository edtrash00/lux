/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "arg.h"
#include "compat.h"
#include "db.h"
#include "fs.h"

/* TODO */
static int
pkg_add(const char *path)
{
	return 0;
}

static int
pkg_del(const char *path)
{
	int rval = 0;
	Package *pkg;
	struct node *current;

	if (!(pkg = open_db(path)))
		err(1, "open_db %s", path);

	for (current = pkg->files; current; current = current->next)
		rval |= fs_remove(current->data);

	for (current = pkg->dirs; current; current = current->next)
		rval |= fs_remove(current->data);

	return rval;
}

static int
pkg_inf(const char *path)
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

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s -a|-d|-i package...\n", getprogname());
}
int
main(int argc, char *argv[])
{
	int (*pkgfcn)(const char *) = NULL;
	int rval = 0;

	ARGBEGIN {
	case 'a':
		pkgfcn = pkg_add;
		break;
	case 'd':
		pkgfcn = pkg_del;
		break;
	case 'i':
		pkgfcn = pkg_inf;
		break;
	default:
		usage();
	} ARGEND

	if (!argv)
		usage();

	for (; *argv; argv++)
		rval |= pkgfcn(*argv);

	return 0;
}
