#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "lux.h"

int
warn_open_db(Package **pkg, const char *path) {
	if (!(*pkg = open_db(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	return 0;
}

int
warn_rm(const char *path) {
	if (remove(path) < 0) {
		warn("rm %s", path);
		return 1;
	}

	return 0;
}

int
warn_mv(const char *src, const char *dest)
{
	char buf[PATH_MAX];

	if (move(src, dest) < 0) {
		if (*dest == '/')
			snprintf(buf, sizeof(buf), "%s%s", dest, src);
		else
			snprintf(buf, sizeof(buf), "%s/%s", dest, src);
		warn("mv %s -> %s", src, buf);
		return 1;
	}

	return 0;
}
