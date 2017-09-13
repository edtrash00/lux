/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "fs.h"

int
move_file(const char *src, const char *dest)
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
