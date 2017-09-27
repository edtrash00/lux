#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lux.h"

int
add(const char *path, Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->dirs; np; np = np->next)
		rval |= mv(np->data, PKG_DIR);
	for (np = pkg->files; np; np = np->next)
		rval |= mv(np->data, PKG_DIR);

	return rval;
}
