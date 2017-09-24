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
#include "pkg.h"

#define FLEN 512
#define ULEN 800

static const char *fmt;

static int
pkg_fetch(const char *path)
{
	char file[FLEN], buf[ULEN], url[PATH_MAX], tmp[PATH_MAX];
	int readcnt, fd = -1, rval = 0;
	Package *pkg;

	if (!(pkg = open_db(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	if ((fd = open(PKG_SRC, O_RDONLY, 0)) < 0) {
		warn("open %s", PKG_SRC);
		goto failure;
	}

	while (read(fd, buf, sizeof(buf)) > 0)
		continue;

	readcnt = strlen(buf);
	buf[readcnt-1] = '\0'; /* remove trailing newline */

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
	close_db(pkg);

	if (fd != -1)
		close(fd);

	return rval;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s package ...\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int rval = 0;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++)
		rval |= pkg_fetch(*argv);

	return rval;
}
