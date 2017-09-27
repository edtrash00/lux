#include "lux.h"

int
del(const char *path, Package *pkg)
{
	int rval = 0;
	struct node *np;

	rval = warn_rm(path);

	for (np = pkg->files; np; np = np->next)
		rval |= warn_rm(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= warn_rm(np->data);

	return rval;
}
