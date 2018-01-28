#include <curl/curl.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fetch.h"
#include "pkg.h"

#define FILEMODE(a) ((a) ? O_RDWR|O_CREAT|O_TRUNC : O_RDONLY)
#define URL_MAX  (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

static int
fetch(Package *pkg)
{
	char buf[BUFSIZ], file[NAME_MAX], url[PATH_MAX], tmp[PATH_MAX];
	int fd[2] = {-1}, i = 0, rval = 0;
	unsigned int lsum, rsum;
	ssize_t rf, fsize = 0;

	for (; i < PKG_NUM; i++) {
		snprintf(file, sizeof(file), "%s#%s%s",
		    pkg->name, pkg->version, (i == 0) ? PKG_FMT : PKG_SIG);
		snprintf(url, sizeof(url), "%.*s/%s", URL_MAX, PKG_SRC, file);
		snprintf(tmp, sizeof(tmp), "%s/%s", PKG_TMP, file);

		if ((fd[i] = open(tmp, FILEMODE(i), DEFFILEMODE)) < 0) {
			warn("open %s", tmp);
			goto failure;
		}

		fetchLastErrCode = 0;
		if (netfd(url, fd[i], NULL) < 0) {
			if (fetchLastErrCode)
				warnx("netfd %s: %s", url, fetchLastErrString);
			else
				warn("netfd %s", tmp);
			goto failure;
		}
	}

	while ((rf = read(fd[1], buf, sizeof(buf))) > 0)
		fsize += rf;
	buf[fsize-1] = '\0';

	lsum = filetosum(fd[0]);
	rsum = stoll(buf, 0, SSIZE_MAX, 10);

	if (lsum != rsum) {
		warnx("fetch %s: checksum mismatch", pkg->name);
		goto failure;
	}

	if (fsize != pkg->pkgsize) {
		warnx("fetch %s: size mismatch", pkg->name);
		goto failure;
	}

	goto done;
failure:
	rval = 1;
done:
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);
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
