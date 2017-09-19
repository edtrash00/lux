#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "pkg.h"

int
mv(const char *src, const char *dest)
{
	char buf[PATH_MAX];
	struct stat st;

	if (lstat(src, &st) < 0) {
		warn("lstat %s", src);
		return 1;
	}

	snprintf(buf, sizeof(buf), "%s/%s", dest, src);

	switch(st.st_mode & S_IFMT) {
	case S_IFDIR:
		if (mkdir(buf, st.st_mode) < 0
		    && errno != EEXIST) {
			warn("mkdir %s", buf);
			return 1;
		}
		break;
	default:
		if (rename(src, buf) < 0) {
			warn("rename %s", buf);
			return 1;
		}
		break;
	}

	return 0;
}

int
wunlink(const char *pathname)
{
	if (unlink(pathname) < 0) {
		warn("unlink %s", pathname);
		return 1;
	}

	return 0;
}

int
wrmdir(const char *pathname)
{
	if (rmdir(pathname) < 0 && errno != ENOTEMPTY) {
		warn("rmdir %s", pathname);
		return 1;
	}

	return 0;
}
