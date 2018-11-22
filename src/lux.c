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
((x) == LOCAL ? PKG_LDB : (x) == REMOTE ? PKG_RDB : (x) == NONE ? "" : NULL)
#define MODERWCT  (O_RDWR|O_CREAT|O_TRUNC)

#define freepool() stackpool.n = 0;

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

#define POOLSIZE 65536
#define PATHMAX  (POOLSIZE/2)

static char buffer[POOLSIZE];
static Membuf stackpool = { sizeof(buffer), 0, buffer };

static int
pnode(Node *np, int isfile, int putch)
{
	Membuf p;

	membuf_strinit(&p, PATHMAX);

	if (isfile)
		membuf_strcat(&p, PKG_DIR);

	for (; np; np = np->next) {
		if (putch++)
			putchar(' ');
		p.n -= membuf_strcat(&p, np->data);
		fputs(p.p, stdout);
	}

	freepool();

	return 1;
}

/* action functions */
static int
add(Package *pkg)
{
	Membuf p1, p2;
	Node *np;
	int i, rval;

	i    = 0;
	rval = 0;

	membuf_strinit(&p1, PATHMAX);
	membuf_strinit(&p2, PATHMAX);
	membuf_vstrcat(&p1, PKG_TMP, pkg->name, "#", pkg->version, "/");
	membuf_strcat(&p2, PKG_DIR);
	for (; i < 2; i++) {
		np = (i == 0) ? pkg->files : pkg->dirs;
		for (; np; np = np->next) {
			p1.n -= membuf_strcat(&p1, np->data);
			p2.n -= membuf_strcat(&p2, np->data);
			if (move(p1.p, p2.p) < 0)
				rval = 1;
		}
	}

	freepool();

	return rval;
}

static int
del(Package *pkg)
{
	Membuf p;
	Node *np;
	int i, rval;

	i    = 0;
	rval = 0;

	membuf_strinit(&p, PATHMAX);
	membuf_strcat(&p, PKG_DIR);
	for (; i < 2; i++) {
		np = (i == 0) ? pkg->dirs : pkg->files;
		for (; np; np = np->next) {
			p.n -= membuf_strcat(&p, np->data);
			if (remove(p.p) < 0)
				rval = 1;
		}
	}

	freepool();

	return rval;
}

static int
explode(Package *pkg)
{
	Membuf p1, p2;
	int fd[2], rval;

	fd[0] = fd[1] = -1;
	rval  = 0;

	membuf_strinit(&p1, PATHMAX);
	membuf_vstrcat(&p1, PKG_TMP, pkg->name, "#", pkg->version);

	if (mkdir(p1.p, ACCESSPERMS) < 0) {
		warn("mkdir %s", p1.p);
		goto failure;
	}

	/* unarchive acts locally */
	if (chdir(p1.p) < 0) {
		warn("chdir %s", p1.p);
		goto failure;
	}

	membuf_vstrcat(&p2, p1.p, ".ustar");
	membuf_strcat(&p1, PKG_FMT);

	if ((fd[0] = open(p1.p, O_RDONLY, 0)) < 0) {
		warn("open %s", p1.p);
		goto failure;
	}

	if ((fd[1] = open(p2.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p2.p);
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
	freepool();
	return rval;
}

static int
fetch(Package *pkg)
{
	Membuf tmp;
	ssize_t rf, fsize, size;
	int fd[2], i, rval;
	unsigned lsum, rsum;
	char buf[LINE_MAX];
	char *p;

	fd[0] = fd[1] = -1;
	fsize = 0;
	i     = 0;
	rval  = 0;

	membuf_strinit(&tmp, PATHMAX);
	for (; i < 2; i++) {
		tmp.n -= membuf_vstrcat(&tmp, pkg->name, "#", pkg->version,
		         (i == 0) ? PKG_SIG : PKG_FMT);
		if ((fd[i] = open(tmp.p, MODERWCT, DEFFILEMODE)) < 0) {
			warn("open %s", tmp.p);
			goto failure;
		}

		tmp.n -= membuf_vstrcat(&tmp, PKG_SRC,
		         pkg->name, "#", pkg->version);
		if (netfd(tmp.p, fd[i], NULL) < 0)
			goto failure;
	}

	lseek(fd[0], 0, SEEK_SET);
	lseek(fd[1], 0, SEEK_SET);

	while ((rf = read(fd[0], buf, sizeof(buf))) > 0)
		fsize += rf;

	buf[fsize-1] = '\0';

	if ((p = strchr(buf, ' ')))
		*p++ = '\0';

	if (!p || !(*p)) {
		warnx("fetch %s: checksum file in wrong format", pkg->name);
		goto failure;
	}

	lsum = filetosum(fd[1], &fsize);
	rsum = strtobase(buf, 0, UINT_MAX,  10);
	size = strtobase(p,   0, SSIZE_MAX, 10);

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
	freepool();
	return rval;
}

static int
regpkg(Package *pkg)
{
	Membuf p;

	membuf_strinit(&p, PATHMAX);
	membuf_vstrcat(&p, GETDB(LOCAL), pkg->name);

	if (copy(pkg->path, p.p) < 0)
		return 1;

	freepool();

	return 0;
}

static int
show_desc(Package *pkg)
{
	if (pkg->description)
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
	if (pkg->license)
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
	if (pkg->name)
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
	if (pkg->version)
		puts(pkg->version);

	return 0;
}

static int
unregpkg(Package *pkg)
{
	if (remove(pkg->path) < 0)
		return 1;

	return 0;
}

static int
update(void)
{
	Membuf p;
	int fd[2], rval;

	fd[0] = fd[1] = -1;
	rval  = 0;

	membuf_strinit(&p, PATHMAX);
	p.n -= membuf_vstrcat(&p, PKG_TMP, PKG_FDB);

	if ((fd[0] = open(p.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p.p);
		goto failure;
	}

	p.n -= membuf_vstrcat(&p, PKG_SDB, PKG_FDB);

	if (netfd(p.p, fd[0], NULL) < 0)
		goto failure;

	p.n -= membuf_vstrcat(&p, PKG_TMP, PKG_FDB, ".ustar");

	if ((fd[1] = open(p.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p.p);
		goto failure;
	}

	lseek(fd[0], 0, SEEK_SET);
	if (uncomp(fd[0], fd[1]) < 0)
		goto failure;

	/* unarchive acts locally */
	if (chdir(PKG_RDB) < 0) {
		warn("chdir %s", PKG_RDB);
		goto failure;
	}

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
	freepool();
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
	Membuf p;
	Package *pkg;
	int (*fn)(Package *);
	int rval, type, atype;
	unsigned hash;

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

	membuf_strinit(&p, PATHMAX);
	membuf_vstrcat(&p, GETDB((atype == 0) ? type : atype), "/");
	for (; *argv; argc--, argv++) {
		p.n -= membuf_strcat(&p, *argv);
		if (!(pkg = db_open(p.p))) {
			if (errno == ENOMEM)
				exit(1);
			rval = 1;
			continue;
		}
		freepool();
		rval |= fn(pkg);
		db_close(pkg);
	}

	return rval;
}
