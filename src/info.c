#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lux.h"
#include "pkg.h"

int
info(const char *path)
{
	Package *pkg;
	struct node *np;

	if (!(pkg = open_db(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	printf(
	    "Name:        %s\n"
	    "Version:     %s\n"
	    "License:     %s\n"
	    "Description: %s\n",
	    pkg->name, pkg->version, pkg->license, pkg->description);
	
	puts("\nDependencies:");
	for (np = pkg->rdeps; np; np = np->next)
		printf("R: %s\n", (char *)np->data);
	for (np = pkg->mdeps; np; np = np->next)
		printf("M: %s\n", (char *)np->data);

	puts("\nFiles:");
	for (np = pkg->dirs; np; np = np->next)
		printf("D: %s\n", (char *)np->data);
	for (np = pkg->files; np; np = np->next)
		printf("M: %s\n", (char *)np->data);
	close_db(pkg);

	return 0;
}
