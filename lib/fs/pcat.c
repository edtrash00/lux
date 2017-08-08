/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "fs.h"

char *
pcat(const char *f1, const char *f2)
{
	static char buf[PATH_MAX];

	if (snprintf(buf, sizeof(buf), "%s/%s", f1, f2) >= PATH_MAX) {
		errno = ENAMETOOLONG;
		err(1, "snprintf %s/%s", f1, f2);
	}

	return buf;
}
