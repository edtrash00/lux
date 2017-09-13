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
	if (unlink(pathname) < 0) {
		warn("unlink %s", pathname);
		return 1;
	}

	return 0;
}

int
remove_dir(const char *pathname)
{
	if (rmdir(pathname) < 0 && errno != ENOTEMPTY) {
		warn("rmdir %s", pathname);
		return 1;
	}

	return 0;
}
