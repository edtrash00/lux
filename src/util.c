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
#define URL_MAX     (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

static int
mvtosys(const char *path)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);

	if (move(path, buf) < 0) {
		warn("move %s -> %s", path, buf);
		return 1;
	}

	return 0;
}

static int
rmfromsys(const char *path)
{
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);

	if (remove(buf) < 0) {
		warn("remove %s", buf);
		return 1;
	}

	return 0;
}

static int
pnode(const char *prefix, struct node *np, int opts)
{
	char path[PATH_MAX], *str;

	for (; np; np = np->next) {
		str = np->data;
		if (opts & PUTCH)
			putchar((opts & PFLAG) ? '\n' : ' ');

		if (opts & PFLAG)
			printf("%s", prefix);

		if (*prefix == 'D' || *prefix == 'F')
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
		else
			snprintf(path, sizeof(path), "%s", str);

		printf("%s", path);
	}

	return (PUTCH);
}

int
add(Package *pkg)
{
	struct node *np;
	int rval;

	rval = 0;

	for (np = pkg->dirs; np; np = np->next)
		rval |= mvtosys(np->data);
	for (np = pkg->files; np; np = np->next)
		rval |= mvtosys(np->data);

	return rval;
}

int
del(Package *pkg)
{
	struct node *np;
	int rval;

	rval = 0;

	for (np = pkg->files; np; np = np->next)
		rval |= rmfromsys(np->data);
	for (np = pkg->dirs; np; np = np->next)
		rval |= rmfromsys(np->data);

	return rval;
}

int
fetch(Package *pkg)
{
	ssize_t rf, fsize;
	int fd[2], i, rval;
	unsigned int lsum, rsum;
	char buf[BUFSIZ], tmp[PATH_MAX], url[PATH_MAX], file[NAME_MAX];

	fd[0] = fd[1] = -1;
	fsize = 0;
	i     = 0;
	rval  = 0;

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

void
info(Package *pkg, int opts)
{
	if (opts & AFLAG)
		printf(
		    "Name:        %s\n"
		    "Version:     %s\n"
		    "License:     %s\n"
		    "Description: %s\n",
		    pkg->name, pkg->version, pkg->license, pkg->description);

	if (opts & RFLAG)
		opts |= pnode("R: ", pkg->rdeps, opts);
	if (opts & MFLAG)
		opts |= pnode("M: ", pkg->mdeps, opts);
	if (opts & DFLAG)
		opts |= pnode("D: ", pkg->dirs,  opts);
	if (opts & FFLAG)
		opts |= pnode("F: ", pkg->files, opts);

	if ((opts & ~AFLAG))
		putchar('\n');
}
