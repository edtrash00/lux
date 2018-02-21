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

#define MODERWCT    (O_RDWR|O_CREAT|O_TRUNC)
#define FILEMODE(a) (((a) == 0) ? MODERWCT : O_RDONLY)
#define URL_MAX     (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

enum Hash {
	ADD     = 97,  /* add        */
	DEL     = 109, /* del        */
	EXPLODE = 111, /* explode    */
	FETCH   = 124, /* fetch      */
	INFO    = 14,  /* info       */
	SHOWFS  = 91,  /* show-files */
	SHOWMD  = 94,  /* show-mdeps */
	SHOWRD  = 53,  /* show-rdeps */
	UPDATE  = 17   /* update     */
};

enum RTypes {
	LOCAL,
	REMOTE,
	NONE
};

static int
pnode(struct node *np, int isfile, int putch)
{
	char *str, path[PATH_MAX];

	for (; np; np = np->next) {
		str = np->data;
		if (putch++)
			putchar(' ');

		if (isfile)
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
		else
			snprintf(path, sizeof(path), "%s", str);

		printf("%s", path);
	}

	return 1;
}

/* action functions */
static int
add(Package *pkg)
{
	struct node *np;
	int i, rval;
	char buf[2][PATH_MAX];

	i    = 0;
	rval = 0;

	for (; i < 2; i++) {
		np = i ? pkg->files : pkg->dirs;
		for (; np; np = np->next) {
			snprintf(buf[0], sizeof(buf[0]), "%s%s#%s/%s",
			         PKG_TMP, pkg->name,
			         pkg->version, (char *)np->data);
			snprintf(buf[1], sizeof(buf[1]), "%s%s",
			         PKG_DIR, (char *)np->data);
			if (move(buf[0], buf[1]) < 0) {
				warn("move %s -> %s", buf[0], buf[1]);
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
	char buf[2][PATH_MAX];

	fd[0] = fd[1] = -1;
	rval  = 0;

	snprintf(buf[0], sizeof(buf[0]), "%s%s#%s",
	         PKG_TMP, pkg->name, pkg->version);

	if (mkdir(buf[0], ACCESSPERMS) < 0) {
		warn("mkdir %s", buf[0]);
		goto failure;
	}

	snprintf(buf[0], sizeof(buf[0]), "%s%s#%s%s",
	         PKG_TMP, pkg->name, pkg->version, PKG_FMT);
	snprintf(buf[1], sizeof(buf[1]), "%s%s#%s.tar",
	         PKG_TMP, pkg->name, pkg->version);

	if ((fd[0] = open(buf[0], O_RDONLY, 0)) < 0) {
		warn("open %s", buf[0]);
		goto failure;
	}
	if ((fd[1] = open(buf[1], MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", buf[1]);
		goto failure;
	}

	if (uncomp(fd[0], fd[1]) < 0) {
		if (z_errno)
			warnx("invalid or incomplete deflate data");
		else
			warn("uncomp %s -> %s", buf[0], buf[1]);
		goto failure;
	}

	lseek(fd[1], 0, SEEK_SET);
	if (unarchive(fd[1]) < 0) {
		warn("unarchive %s", buf[1]);
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
	char url[PATH_MAX], file[NAME_MAX];
	char buf[BUFSIZ], tmp[PATH_MAX];

	fd[0] = fd[1] = -1;
	fsize = 0;
	i     = 0;
	rval  = 0;

	for (; i < 2; i++) {
		snprintf(file, sizeof(file), "%s#%s%s",
		         pkg->name, pkg->version, (i == 0) ? PKG_FMT : PKG_SIG);
		snprintf(url, sizeof(url), "%.*s%s", URL_MAX, PKG_SRC, file);
		snprintf(tmp, sizeof(tmp), "%s%s", PKG_TMP, file);

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
	printf("Name:        %s\n"
	       "Version:     %s\n"
	       "License:     %s\n"
	       "Description: %s\n",
	       pkg->name, pkg->version, pkg->license, pkg->description);
	return 0;
}

static int
showfiles(Package *pkg)
{
	pnode(pkg->dirs,  1, 0);
	pnode(pkg->files, 1, 1);
	return 0;
}

static int
showmdeps(Package *pkg)
{
	pnode(pkg->mdeps, 0, 0);
	return 0;
}

static int
showrdeps(Package *pkg)
{
	pnode(pkg->rdeps, 0, 0);
	return 0;
}

static int
update(Package *pkg)
{
	int fd[2], rval;
	char buf[PATH_MAX], tmp[PATH_MAX];

	fd[0] = fd[1] = -1;
	rval  = 0;

	snprintf(buf, sizeof(buf), "%sdb.%s", PKG_RDB, PKG_FMT);

	if ((fd[0] = open(buf, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", buf);
		goto failure;
	}

	snprintf(tmp, sizeof(tmp), "%.*sdb.%s", URL_MAX, PKG_SRC, PKG_FMT);

	if (netfd(tmp, fd[0], NULL) < 0) {
		if (fetchLastErrCode)
			warnx("netfd %s: %s", tmp, fetchLastErrString);
		else
			warn("netfd %s", buf);
		goto failure;
	}

	snprintf(tmp, sizeof(tmp), "%sdb.tar", PKG_RDB);

	if ((fd[1] = open(tmp, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", tmp);
		goto failure;
	}

	if (uncomp(fd[0], fd[1]) < 0) {
		if (z_errno)
			warnx("invalid or incomplete deflate data");
		else
			warn("uncomp %s -> %s", buf, tmp);
		goto failure;
	}

	lseek(fd[1], 0, SEEK_SET);
	if (unarchive(fd[1]) < 0) {
		warn("unarchive %s", tmp);
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
	fprintf(stderr, "usage: %s [-LNR] command package ...", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	Package *pkg;
	unsigned hash;
	int (*fn)(Package *);
	int rval, type, atype;
	char buf[2][PATH_MAX];

	atype = 0;
	rval  = 0;
	setprogname(argv[0]);

	ARGBEGIN {
	case 'L':
		atype = LOCAL;
		break;
	case 'N':
		atype = NONE;
		break;
	case 'R':
		atype = REMOTE;
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
		break;
	case SHOWFS:
		fn   = showfiles;
		type = LOCAL;
	case SHOWMD:
		fn   = showmdeps;
		type = LOCAL;
		break;
	case SHOWRD:
		fn   = showrdeps;
		type = LOCAL;
		break;
	case UPDATE:
		fn   = update;
		type = REMOTE;
	default:
		usage();
	}

	argc--, argv++;
	type = atype ? atype : type;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf[0], sizeof(buf[0]), "%s/%s", GETDB(type), *argv);
		if (!(pkg = db_open(buf[0]))) {
			if (errno == ENOMEM)
				err(1, NULL);
			warn("open_db %s", buf[0]);
			rval = 1;
			continue;
		}
		rval |= fn(pkg);
		db_close(pkg);

		/* update database state */
		switch (hash) {
		case ADD:
			snprintf(buf[1], sizeof(buf[1]), "%s/%s",
			         GETDB(LOCAL), *argv);
			if (copy(buf[0], buf[1]) < 0)
				err(1, "copy %s -> %s", buf[0], buf[1]);
			break;
		case DEL:
			if (remove(buf[0]) < 0)
				err(1, "remove %s", buf[0]);
			break;
		}
	}

	return rval;
}
