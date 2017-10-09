#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "fetch.h"
#include "lux.h"

#define FMT     PKG_FMT
#define URL_MAX 800

static int
fetch(Package *pkg)
{
	char file[NAME_MAX], buf[URL_MAX], url[PATH_MAX], tmp[PATH_MAX];
	int fd = -1, rval = 0;
	ssize_t readcnt, bsize = 0;

	if ((fd = open(PKG_SRC, O_RDONLY, 0)) < 0) {
		warn("open %s", PKG_SRC);
		goto failure;
	}

	while ((readcnt = read(fd, buf, sizeof(buf))) > 0)
		bsize += readcnt;

	if (readcnt < 0) {
		warn("read %s", PKG_SRC);
		goto failure;
	}

	buf[bsize-1] = '\0'; /* remove trailing newline */

	snprintf(file, sizeof(file), "%s-%s%s", pkg->name, pkg->version, FMT);
	snprintf(url, sizeof(url), "%s/%s", buf, file);
	snprintf(tmp, sizeof(tmp), "%s/%s", PKG_TMP, file);

	if (download(url, tmp, NULL) < 0) {
		if (fetchLastErrString)
			warnx("download %s: %s", url, fetchLastErrString);
		else
			warn("download %s", tmp);
		goto failure;
	}

	goto done;
failure:
	rval = 1;
done:
	if (fd != -1)
		close(fd);

	return rval;
}

int
fetch_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int rval = 0;
	Package *pkg;

	argc--, argv++; /* remove comand string */

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
