#include <err.h>

#include "lux.h"

int
add(const char *path, Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->dirs; np; np = np->next)
		rval |= warn_mv(np->data, PKG_DIR);
	for (np = pkg->files; np; np = np->next)
		rval |= warn_mv(np->data, PKG_DIR);

	if (copy(path, PKG_LDB) < 0) {
		warn("cp %s -> %s", path, PKG_LDB);
		return 1;
	}

	return rval;
}
