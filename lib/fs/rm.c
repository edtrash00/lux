/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <unistd.h>

int
remove_file(const char *pathname)
{
	int rval = 0;

	if (unlink(pathname) < 0) {
		warn("unlink %s", pathname);
		rval = 1;
	}

	return rval;
}

int
remove_dir(const char *pathname)
{
	int rval = 0;

	if (rmdir(pathname) < 0 && errno != ENOTEMPTY) {
		warn("rmdir %s", pathname);
		rval = 1;
	}

	return rval;
}
