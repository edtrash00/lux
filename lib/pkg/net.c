#include <err.h>
#include <stdio.h>
#include <unistd.h>

#include "fetch.h"

int
netfd(char *URL, int fd, const char *flags)
{
	char buf[BUFSIZ];
	int rval = 0;
	ssize_t readcnt;
	struct fetchIO *f = NULL;
	struct url *url = NULL;

	if (!(url = fetchParseURL(URL))) {
		warnx("fetchParseURL %s: %s", URL, fetchLastErrString);
		goto failure;
	}

	if (!(f = fetchGet(url, flags))) {
		warnx("fetchGet %s: %s", URL, fetchLastErrString);
		goto failure;
	}

	while ((readcnt = fetchIO_read(f, buf, sizeof(buf))) > 0) {
		if (write(fd, buf, readcnt) != readcnt) {
			warn("write");
			goto failure;
		}
	}

	if (readcnt < 0) {
		warn("read");
		goto failure;
	}

	goto done;
failure:
	rval = -1;
done:
	if (f)
		fetchIO_close(f);
	if (url)
		fetchFreeURL(url);

	return rval;
}
