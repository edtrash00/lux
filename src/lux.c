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

/* S  for snprintf
 * SN for string concatenation */
#define S(a, b, ...) \
snprintf((a), sizeof((a)), (b), __VA_ARGS__)
#define SN(a, b, c, ...) \
snprintf((a)+(b), sizeof((a))-(b), (c), __VA_ARGS__)

#define MODERWCT    (O_RDWR|O_CREAT|O_TRUNC)
#define FILEMODE(a) (((a) == 0) ? MODERWCT : O_RDONLY)
#define URL_MAX     (URL_HOSTLEN + URL_SCHEMELEN + URL_USERLEN + URL_PWDLEN)

enum Hash {
	ADD     = 30881, /* add               */
	DEL     = 33803, /* del               */
	EXPLODE = 63713, /* explode           */
	FETCH   = 1722,  /* fetch             */
	REG     = 11939, /* register          */
	SDESC   = 56620, /* show-description  */
	SFILES  = 47015, /* show-files        */
	SFLAGS  = 60343, /* show-flags        */
	SLIC    = 62833, /* show-license      */
	SMDEPS  = 64865, /* show-mdeps        */
	SNAME   = 5979,  /* show-name         */
	SRDEPS  = 29414, /* show-rdeps        */
	SVER    = 36360, /* show-version      */
	UNREG   = 37948, /* unregister        */
	UPDATE  = 14537  /* update            */
};

enum RTypes {
	LOCAL  = 1,
	REMOTE = 2,
	NONE   = 3
};

static int
pnode(struct node *np, int isfile, int putch)
{
	size_t n;
	char path[PATH_MAX];

	n = isfile ? S(path, "%s", PKG_DIR) : 0;
	for (; np; np = np->next) {
		if (putch++)
			putchar(' ');

		SN(path, n, "%s", (char *)np->data);
		printf("%s", path);
	}

	return 1;
}

/* action functions */
static int
add(Package *pkg)
{
	struct node *np;
	size_t n[2];
	int i, rval;
	char buf[2][PATH_MAX];

	i    = 0;
	rval = 0;

	n[0] = S(buf[0], "%s%s#%s/", PKG_TMP, pkg->name, pkg->version);
	n[1] = S(buf[1], "%s", PKG_DIR);
	for (; i < 2; i++) {
		np = i ? pkg->files : pkg->dirs;
		for (; np; np = np->next) {
			SN(buf[0], n[0], "%s", (char *)np->data);
			SN(buf[1], n[1], "%s", (char *)np->data);
			if (move(buf[0], buf[1]) < 0)
				rval = 1;
		}
	}

	return rval;
}

static int
del(Package *pkg)
{
	struct node *np;
	size_t n;
	int i, rval;
	char buf[BUFSIZ];

	i    = 0;
	rval = 0;

	n = S(buf, "%s", PKG_DIR);
	for (; i < 2; i++) {
		np = i ? pkg->dirs : pkg->files;
		for (; np; np = np->next) {
			SN(buf, n, "%s", (char *)np->data);
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
	size_t n;
	int fd[2], rval;
	char buf[2][PATH_MAX];

	fd[0] = fd[1] = -1;
	rval  = 0;

	n = S(buf[0], "%s%s#%s", PKG_TMP, pkg->name, pkg->version);

	if (mkdir(buf[0], ACCESSPERMS) < 0) {
		warn("mkdir %s", buf[0]);
		goto failure;
	}

	/* unarchive acts locally */
	if (chdir(buf[0]) < 0) {
		warn("chdir %s", buf[0]);
		goto failure;
	}

	S(buf[1], "%s.tar", buf[0]);
	SN(buf[0], n, "%s", PKG_FMT);

	if ((fd[0] = open(buf[0], O_RDONLY, 0)) < 0) {
		warn("open %s", buf[0]);
		goto failure;
	}
	if ((fd[1] = open(buf[1], MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", buf[1]);
		goto failure;
	}

	if (uncomp(fd[0], fd[1]) < 0)
		goto failure;

	lseek(fd[1], 0, SEEK_SET);
	if (unarchive(fd[1]) < 0)
		goto failure;

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
	ssize_t rf, fsize, size;
	int fd[2], i, rval;
	unsigned n, lsum, rsum;
	char *p;
	char buf[LINE_MAX], tmp[PATH_MAX], file[NAME_MAX];

	fd[0] = fd[1] = -1;
	fsize = 0;
	i     = 0;
	rval  = 0;

	n = S(file, "%s%s", pkg->name, pkg->version);
	for (; i < 2; i++) {
		SN(file, n, "%s", i ? PKG_SIG : PKG_FMT);
		S(tmp, "%s%s", PKG_TMP, file);
		if ((fd[i] = open(tmp, FILEMODE(i), DEFFILEMODE)) < 0) {
			warn("open %s", tmp);
			goto failure;
		}

		S(tmp, "%.*s%s", URL_MAX, PKG_SRC, file);
		if (netfd(tmp, fd[i], NULL) < 0)
			goto failure;
	}

	while ((rf = read(fd[1], buf, sizeof(buf))) > 0)
		fsize += rf;

	buf[fsize-1] = '\0';

	if ((p = strchr(buf, ' ')))
		*p++ = '\0';

	if (!p || !(*p)) {
		warnx("fetch %s: checksum file in wrong format", pkg->name);
		goto failure;
	}

	lsum = filetosum(fd[0]);
	rsum = strtobase(buf, 0, UINT_MAX, 10);
	size = strtobase(p, 0, UINT_MAX, 10);

	if (fsize != size) {
		warnx("fetch %s: size mismatch", pkg->name);
		goto failure;
	}

	if (lsum != rsum) {
		warnx("fetch %s: checksum mismatch", pkg->name);
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
regpkg(Package *pkg)
{
	int rval;
	char buf[PATH_MAX];

	S(buf, "%s%s", GETDB(LOCAL), pkg->name);
	rval = copy(pkg->path, buf);

	return (rval < 0);
}

static int
show_desc(Package *pkg)
{
	puts(pkg->description);
	return 0;
}

static int
show_files(Package *pkg)
{
	pnode(pkg->dirs,  1, 0);
	pnode(pkg->files, 1, 1);
	return 0;
}

static int
show_flags(Package *pkg)
{
	pnode(pkg->flags, 0, 0);
	return 0;
}

static int
show_lic(Package *pkg)
{
	puts(pkg->license);
	return 0;
}

static int
show_mdeps(Package *pkg)
{
	pnode(pkg->mdeps, 0, 0);
	return 0;
}

static int
show_name(Package *pkg)
{
	puts(pkg->name);
	return 0;
}

static int
show_rdeps(Package *pkg)
{
	pnode(pkg->rdeps, 0, 0);
	return 0;
}

static int
show_ver(Package *pkg)
{
	puts(pkg->version);
	return 0;
}

static int
unregpkg(Package *pkg)
{
	int rval;
	rval = remove(pkg->path);
	return (rval < 0);
}

static int
update(void)
{
	int fd[2], rval;
	char buf[PATH_MAX];

	fd[0] = fd[1] = -1;
	rval  = 0;

	S(buf, "%s%s%s", PKG_RDB, PKG_FDB, PKG_FMT);

	if ((fd[0] = open(buf, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", buf);
		goto failure;
	}

	S(buf, "%.*s%s%s", URL_MAX, PKG_SRC, PKG_FDB, PKG_FMT);

	if (netfd(buf, fd[0], NULL) < 0)
		goto failure;

	S(buf, "%s%s.tar", PKG_RDB, PKG_FDB);

	if ((fd[1] = open(buf, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", buf);
		goto failure;
	}

	if (uncomp(fd[0], fd[1]) < 0)
		goto failure;

	lseek(fd[1], 0, SEEK_SET);
	if (unarchive(fd[1]) < 0)
		goto failure;

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
	fprintf(stderr, "usage: %s [-LNR] command package ...\n",
	        getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	Package *pkg;
	size_t n;
	int (*fn)(Package *);
	int rval, type, atype;
	unsigned int hash;
	char buf[PATH_MAX];

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

	if (!argc)
		usage();

	switch ((hash = strtohash(*argv))) {
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
	case REG:
		fn   = regpkg;
		type = REMOTE;
		break;
	case SDESC:
		fn   = show_desc;
		type = LOCAL;
		break;
	case SFILES:
		fn   = show_files;
		type = LOCAL;
		break;
	case SFLAGS:
		fn   = show_flags;
		type = LOCAL;
		break;
	case SLIC:
		fn   = show_lic;
		type = LOCAL;
		break;
	case SMDEPS:
		fn   = show_mdeps;
		type = LOCAL;
		break;
	case SNAME:
		fn   = show_name;
		type = LOCAL;
		break;
	case SRDEPS:
		fn   = show_rdeps;
		type = LOCAL;
		break;
	case SVER:
		fn   = show_ver;
		type = LOCAL;
		break;
	case UNREG:
		fn   = unregpkg;
		type = LOCAL;
		break;
	case UPDATE:
		rval = update();
		exit(rval);
	default:
		usage();
	}

	argc--, argv++;

	n = S(buf, "%s", GETDB(atype ? atype : type));
	for (; *argv; argc--, argv++) {
		SN(buf, n, "%s", *argv);
		if (!(pkg = db_open(buf))) {
			if (errno == ENOMEM)
				exit(1);
			rval = 1;
			continue;
		}
		rval |= fn(pkg);
		db_close(pkg);
	}

	return rval;
}
