#include <err.h>
#include <stdio.h>
#include <unistd.h>

#include "fetch.h"
#include "pkg.h"

int
netfd(char *URL, int fd, const char *flags)
{
	char buf[BUFSIZ];
	int rval;
	ssize_t readcnt, total;
	struct fetchIO *f;
	struct url *url;
	struct url_stat us;

	f                = NULL;
	fetchLastErrCode = 0;
	url              = NULL;
	rval             = 0;
	total            = 0;

	if (!(url = fetchParseURL(URL))) {
		warnx("fetchParseURL %s: %s", URL, fetchLastErrString);
		goto failure;
	}

	if (!(f = fetchXGet(url, &us, flags))) {
		warnx("fetchGet %s: %s", URL, fetchLastErrString);
		goto failure;
	}

	PBINIT();
	while ((readcnt = fetchIO_read(f, buf, sizeof(buf))) > 0) {
		total += readcnt;
		PBUPDATE(total, us.size);
		if (write(fd, buf, readcnt) != readcnt) {
			warn("write");
			goto failure;
		}
	}
	PBEND();

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
