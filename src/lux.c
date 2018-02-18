#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fetch.h"
#include "pkg.h"

#define GETDB(x) \
((x) == LOCAL ? PKG_LDB : (x) == REMOTE ? PKG_RDB : (x) == NONE ? "." : NULL)

#define FILEMODE(a) ((a) ? O_RDWR|O_CREAT|O_TRUNC : O_RDONLY)
#define URL_MAX     (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

enum IFlags {
	AFLAG = 0x01, /* print about      */
	DFLAG = 0x02, /* list directories */
	FFLAG = 0x04, /* list files       */
	MFLAG = 0x08, /* list run deps    */
	PFLAG = 0x10, /* print prefix     */
	RFLAG = 0x20, /* list make deps   */
	PUTCH = 0x40  /* print space      */
};

enum Hash {
	ADD     = 97,  /* add     */
	DEL     = 109, /* del     */
	EXPLODE = 111, /* explode */
	FETCH   = 124, /* fetch   */
	INFO    = 14   /* info    */
};

enum RTypes {
	LOCAL,
	REMOTE,
	NONE
};

static int opts;

static int
pnode(const char *prefix, struct node *np)
{
	char *str, path[PATH_MAX];

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

static int
add(Package *pkg)
{
	struct node *np;
	int i, rval;
	char ibuf[PATH_MAX], obuf[PATH_MAX];

	i    = 0;
	rval = 0;

	for (; i < 2; i++) {
		np = i ? pkg->files : pkg->dirs;
		for (; np; np = np->next) {
			snprintf(ibuf, sizeof(ibuf), "%s%s#%s/%s",
			         PKG_TMP, pkg->name,
			         pkg->version, (char *)np->data);
			snprintf(obuf, sizeof(obuf), "%s%s",
			         PKG_DIR, (char *)np->data);
			if (move(ibuf, obuf) < 0) {
				warn("move %s -> %s", ibuf, obuf);
				rval = 1;
			}
		}
	}

	return rval;
}

static int
del(Package *pkg)
{
	struct node *np;
	int i, rval;
	char buf[BUFSIZ];

	i    = 0;
	rval = 0;

	for (; i < 2; i++) {
		np = i ? pkg->dirs : pkg->files;
		for (; np; np = np->next) {
			snprintf(buf, sizeof(buf), "%s%s",
			         PKG_DIR, (char *)np->data);
			if (remove(buf) < 0) {
				warn("remove %s", buf);
				rval = 1;
			}
		}
	}

	return rval;
}

static int
explode(Package *pkg)
{
	int fd[2], rval;
	char ibuf[PATH_MAX], obuf[PATH_MAX];

	fd[0] = fd[1] = -1;
	rval  = 0;

	snprintf(ibuf, sizeof(ibuf), "%s/%s#%s",
	         PKG_TMP, pkg->name, pkg->version);

	if (mkdir(ibuf, ACCESSPERMS) < 0) {
		warn("mkdir %s", ibuf);
		goto failure;
	}

	snprintf(ibuf, sizeof(ibuf), "%s/%s#%s%s",
	         PKG_TMP, pkg->name, pkg->version, PKG_FMT);
	snprintf(obuf, sizeof(obuf), "%s/%s#%s.tar",
	         PKG_TMP, pkg->name, pkg->version);

	if ((fd[0] = open(ibuf, O_RDONLY, 0)) < 0) {
		warn("open %s", ibuf);
		goto failure;
	}
	if ((fd[1] = open(obuf, O_RDWR | O_CREAT | O_TRUNC, DEFFILEMODE)) < 0) {
		warn("open %s", obuf);
		goto failure;
	}

	if (uncomp(fd[0], fd[1]) < 0) {
		if (z_errno)
			warnx("invalid or incomplete deflate data");
		else
			warn("uncomp %s -> %s", ibuf, obuf);
		goto failure;
	}

	lseek(fd[1], 0, SEEK_SET);
	if (unarchive(fd[1]) < 0) {
		warn("unarchive %s", obuf);
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

static int
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

static int
info(Package *pkg)
{
	if (opts & AFLAG)
		printf(
		    "Name:        %s\n"
		    "Version:     %s\n"
		    "License:     %s\n"
		    "Description: %s\n",
		    pkg->name, pkg->version, pkg->license, pkg->description);

	if (opts & RFLAG)
		opts |= pnode("R: ", pkg->rdeps);
	if (opts & MFLAG)
		opts |= pnode("M: ", pkg->mdeps);
	if (opts & DFLAG)
		opts |= pnode("D: ", pkg->dirs);
	if (opts & FFLAG)
		opts |= pnode("F: ", pkg->files);

	if ((opts & ~AFLAG))
		putchar('\n');

	return 0;
}

static void
usage(void)
{
	fprintf(stderr,
	        "usage: %s [-LNR] add|del|explode|fetch|info [opts] pkg...\n",
	        getprogname());
	exit(1);
}

static int
setoinfo(int argc, char *argv[])
{
	int oldc;

	oldc = argc;

	ARGBEGIN {
	case 'a':
		opts |= AFLAG;
		break;
	case 'd':
		opts |= DFLAG;
		break;
	case 'f':
		opts |= FFLAG;
		break;
	case 'm':
		opts |= MFLAG;
		break;
	case 'p':
		opts |= PFLAG;
		break;
	case 'r':
		opts |= RFLAG;
		break;
	default:
		usage();
	} ARGEND

	if (!opts)
		opts |= (AFLAG|DFLAG|FFLAG|MFLAG|PFLAG|RFLAG);

	return oldc - argc;
}


int
main(int argc, char *argv[])
{
	Package *pkg;
	unsigned hash;
	int (*fn)(Package *);
	int onum, rval, type;
	char *p, buf[PATH_MAX];

	onum = 0;
	rval = 0;
	setprogname(argv[0]);

	ARGBEGIN {
	case 'L':
		type = LOCAL;
		break;
	case 'N':
		type = NONE;
		break;
	case 'R':
		type = REMOTE;
		break;
	default:
		usage();
	} ARGEND

	hash = strtohash(*argv);
	switch (hash) {
	case ADD:
		fn   = add;
		type = REMOTE;
		break;
	case DEL:
		fn   = del;
		type = LOCAL;
		break;
	case EXPLODE:
		fn   = explode;
		type = REMOTE;
		break;
	case FETCH:
		fn   = fetch;
		type = REMOTE;
		break;
	case INFO:
		fn   = info;
		type = LOCAL;
		onum = setoinfo(argc, argv);
		break;
	default:
		usage();
	}

	argc = onum ? argc - onum : argc - 1;
	argv = onum ? argv + onum : argv + 1;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(type), *argv);
		if (!(pkg = db_open(*argv))) {
			if (errno == ENOMEM)
				err(1, NULL);
			warn("open_db %s", *argv);
			rval = 1;
			continue;
		}
		rval |= fn(pkg);
		db_close(pkg);
	}

	/* update database state */
	switch (hash) {
	case ADD:
		if (!(p = strdup(buf)))
			err(1, "strdup");
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(LOCAL), *argv);
		if (copy(p, buf) < 0)
			err(1, "copy %s -> %s", p, buf);
		break;
	case DEL:
		if (remove(buf) < 0)
			err(1, "remove %s", buf);
		break;
	}

	return rval;
}
