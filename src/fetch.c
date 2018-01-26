#include <curl/curl.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"

enum {
	URL_HOSTLEN   = 255,
	URL_SCHEMELEN = 16,
	URL_USERLEN   = 256,
	URL_PWDLEN    = 256,
	URL_MAX       = (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)
};

static int
fetch(Package *pkg)
{
	FILE *fp[2] = { NULL };
	char buf[BUFSIZ], file[NAME_MAX], url[PATH_MAX], tmp[PATH_MAX];
	int i = 0, rval = 0;
	ssize_t lhash, rhash;

	for (; i < PKG_NUM; i++) {
		snprintf(file, sizeof(file), "%s-%s%s",
		    pkg->name, pkg->version, !i ? PKG_FMT : PKG_SIG);
		snprintf(url, sizeof(url), "%.*s/%s", URL_MAX, PKG_SRC, file);
		snprintf(tmp, sizeof(tmp), "%s/%s", PKG_TMP, file);

		if (!(fp[i] = fopen(tmp, "rw"))) {
			warn("fopen %s", tmp);
			goto failure;
		}

		if (download(url, fp[i], NULL) < 0) {
			if (curl_errno)
				warnx("download %s: %s",
				    url, curl_easy_strerror(curl_errno));
			else
				warn("download %s", tmp);
			goto failure;
		}
	}

	fread(buf, sizeof(buf), sizeof(char), fp[1]);
	buf[strlen(buf)-1] = '\0';

	lhash = filetohash(fp[0]);
	rhash = stoll(buf, 0, SIZE_MAX, 10);

	if (lhash != rhash) {
		warnx("download %s: failed checksum", pkg->name);
		goto failure;
	}

	/* fix later, fileno is not going to work as a fd */
	if (unarchive(fileno(fp[0])) < 0)
		err(1, "unarchive %s", pkg->name);

	goto done;
failure:
	rval = 1;
done:
	if (fp[0] != NULL)
		fclose(fp[0]);
	if (fp[1] != NULL)
		fclose(fp[1]);
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
