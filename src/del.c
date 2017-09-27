#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lux.h"

int
del(const char *path, Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->files; np; np = np->next)
		rval |= unlink(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= rmdir(np->data);

	return rval;
}
