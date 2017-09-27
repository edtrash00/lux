#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fetch.h"
#include "lux.h"

#define FLEN 512
#define ULEN 800

static const char *fmt = PKG_FMT;

int
fetch(const char *path, Package *pkg)
{
	char file[FLEN], buf[ULEN], url[PATH_MAX], tmp[PATH_MAX];
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

	snprintf(file, sizeof(file), "%s-%s%s", pkg->name, pkg->version, fmt);
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
