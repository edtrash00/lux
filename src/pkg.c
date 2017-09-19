#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "pkg.h"

static int
add(const char *path)
{
	int rval = 0;
	Package *pkg;
	struct node *current;

	if (!(pkg = open_db(path)))
		err(1, "open_db %s", path);

	rval = mv(path, PKG_DIR);
	for (current = pkg->dirs; current; current = current->next)
		rval |= mv(current->data, PKG_DIR);
	for (current = pkg->files; current; current = current->next)
		rval |= mv(current->data, PKG_DIR);
	close_db(pkg);

	return rval;
}

static int
del(const char *path)
{
	int rval = 0;
	Package *pkg;
	struct node *current;

	if (!(pkg = open_db(path)))
		err(1, "open_db %s", path);

	rval = wunlink(path);
	for (current = pkg->files; current; current = current->next)
		rval |= wunlink(current->data);
	for (current = pkg->dirs; current; current = current->next)
		rval |= wrmdir(current->data);
	close_db(pkg);

	return rval;
}

static int
info(const char *path)
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
		printf("M: %s\n", (char *)current->data);
	close_db(pkg);

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s -adi package...\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int (*pkg)(const char *), rval = 0;

	setprogname(argv[0]);

	ARGBEGIN {
	case 'a':
		pkg = add;
		break;
	case 'd':
		pkg = del;
		break;
	case 'i':
		pkg = info;
		break;
	default:
		usage();
	} ARGEND

	if (!argc || !pkg)
		usage();

	for (; *argv; argv++)
		rval |= pkg(*argv);

	return 0;
}
