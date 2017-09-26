#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fetch.h"

int
download(char *URL, const char *path, const char *flags)
{
	char buf[BUFSIZ];
	int rval = 0, fd = -1;
	ssize_t readcnt;
	struct fetchIO *f = NULL;
	struct url *url = NULL;

	if (!(url = fetchParseURL(URL)))
		goto failure;

	if (!(f = fetchGet(url, flags)))
		goto failure;

	if ((fd = open(path, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0)
		goto failure;

	while ((readcnt = fetchIO_read(f, buf, sizeof(buf))) > 0)
		if (write(fd, buf, readcnt) != readcnt)
			goto failure;

	if (readcnt < 0)
		goto failure;

	goto done;
failure:
	rval = -1;
done:
	if (f)
		fetchIO_close(f);
	if (fd != -1)
		close(fd);
	if (url)
		fetchFreeURL(url);

	return rval;
}
