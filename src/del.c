#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lux.h"
#include "pkg.h"

int
del(const char *path)
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

	rval = wunlink(path);
	for (np = pkg->files; np; np = np->next)
		rval |= wunlink(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= wrmdir(np->data);
	close_db(pkg);

	return rval;
}
