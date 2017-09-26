#include <sys/stat.h>

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

	if (lstat(src, &st) < 0)
		return -1;

	snprintf(buf, sizeof(buf), "%s/%s", dest, src);

	switch(st.st_mode & S_IFMT) {
	case S_IFDIR:
		if (mkdir(buf, st.st_mode) < 0
		    && errno != EEXIST)
			return -1;
		break;
	default:
		if (rename(src, buf) < 0)
			return -1;
		break;
	}

	return 0;
}

