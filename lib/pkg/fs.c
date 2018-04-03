#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"

int
move(const char *src, const char *dest)
{
	struct stat st;

	if (lstat(src, &st) < 0) {
		warn("stat %s", src);
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		if (mkdirp((char *)dest, st.st_mode, ACCESSPERMS) < 0)
			return -1;
	} else if (rename(src, dest) < 0) {
		warn("rename %s %s", src, dest);
		return -1;
	}

	return 0;
}

int
remove(const char *path) {
	struct stat st;
	int (*fn)(const char *);

	if (lstat(path, &st) < 0) {
		warn("stat %s", path);
		return -1;
	}

	if (S_ISDIR(st.st_mode))
		fn = rmdir;
	else
		fn = unlink;

	if (fn(path) < 0) {
		warn("remove %s", path);
		return -1;
	}

	return 0;
}
