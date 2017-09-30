#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fetch.h"
#include "lux.h"

#define FLEN 512
#define ULEN 800

enum Fn {
	ADD   = 0,
	DEL   = 1,
	FETCH = 2,
	INFO  = 3
};

static const char *fmt = PKG_FMT;

static int
add(Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->dirs; np; np = np->next)
		rval |= move(np->data, PKG_DIR);
	for (np = pkg->files; np; np = np->next)
		rval |= move(np->data, PKG_DIR);

	return rval;
}

static int
del(Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->files; np; np = np->next)
		rval |= remove(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= remove(np->data);

	return rval;
}

static int
fetch(Package *pkg)
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

static int
info(Package *pkg)
{
	struct node *np;

	printf(
	    "Name:        %s\n"
	    "Version:     %s\n"
	    "License:     %s\n"
	    "Description: %s\n",
	    pkg->name, pkg->version, pkg->license, pkg->description);

	puts("\nDependencies:");
	for (np = pkg->rdeps; np; np = np->next)
		printf("R: %s\n", (char *)np->data);
	for (np = pkg->mdeps; np; np = np->next)
		printf("M: %s\n", (char *)np->data);

	puts("\nFiles:");
	for (np = pkg->dirs; np; np = np->next)
		printf("D: %s\n", (char *)np->data);
	for (np = pkg->files; np; np = np->next)
		printf("M: %s\n", (char *)np->data);

	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s add   package ...\n"
	    "       %s del   package ...\n"
	    "       %s fetch package ...\n"
	    "       %s info  package ...\n",
	    getprogname(), getprogname(), getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int (*fn)(Package *), rval = 0;
	Package *pkg;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	if (!strcmp(*argv, "add"))
		fn = add;
	else if (!strcmp(*argv, "del"))
		fn = del;
	else if (!strcmp(*argv, "fetch"))
		fn = fetch;
	else if (!strcmp(*argv, "info"))
		fn = info;
	else
		usage();

	argc--, argv++;

	for (; *argv; argc--, argv++) {
		if (!(pkg = open_db(*argv))) {
			if (errno == ENOMEM)
				err(1, NULL);
			rval = 1;
			warn("open_db %s", *argv);
			continue;
		}
		rval |= fn(pkg);
		close_db(pkg);
	}

	return rval;
}
