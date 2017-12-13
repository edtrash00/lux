#include <err.h>
#include <errno.h>

#include "pkg.h"

int
db_eopen(const char *path, Package **pkg)
{
	if (!(*pkg = db_open(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	return 0;
}
