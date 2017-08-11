/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "fs.h"

int
move_file(const char *src, const char *dest)
{
	char buf[PATH_MAX];
	int rval = 0;
	struct stat st;

	if (lstat(src, &st) < 0) {
		warn("lstat %s", src);
		goto failure;
	}

	snprintf(buf, sizeof(buf), "%s/%s", dest, src);

	switch(st.st_mode & S_IFMT) {
	case S_IFDIR:
		if (mkdir(buf, st.st_mode) < 0
		    && errno != EEXIST) {
			warn("mkdir %s", buf);
			goto failure;
		}
		break;
	default:
		if (rename(src, buf) < 0) {
			warn("rename %s", buf);
			goto failure;
		}
		break;
	}

	goto done;
failure:
	rval = 1;
done:
	return rval;
}
