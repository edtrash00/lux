#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fetch.h"
#include "pkg.h"

#define FMT     PKG_FMT
#define URL_MAX (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

static int
fetch(Package *pkg)
{
	char file[NAME_MAX], buf[URL_MAX], url[PATH_MAX], tmp[PATH_MAX];
	int rval = 0;

	snprintf(buf, sizeof(buf), "%s", PKG_SRC);
	snprintf(file, sizeof(file), "%s-%s%s", pkg->name, pkg->version, FMT);
	snprintf(url, sizeof(url), "%s/%s", buf, file);
	snprintf(tmp, sizeof(tmp), "%s/%s", PKG_TMP, file);

	fetchLastErrCode = 0;
	if (download(url, tmp, NULL) < 0) {
		if (fetchLastErrCode)
			warnx("download %s: %s", url, fetchLastErrString);
		else
			warn("download %s", tmp);
		goto failure;
	}

	goto done;
failure:
	rval = 1;
done:
	return rval;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s fetch package ...\n", getprogname());
	exit(1);
}

int
fetch_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int rval = 0;
	Package *pkg;

	argc--, argv++; /* remove comand string */

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(REMOTE), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= fetch(pkg);
		db_close(pkg);
	}

	return rval;
}
