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
#define MODERWCT (O_RDWR|O_CREAT|O_TRUNC)

enum Hash {
	ADD     = 30881, /* add               */
	DEL     = 33803, /* del               */
	EXPLODE = 63713, /* explode           */
	FETCH   = 1722,  /* fetch             */
	REG     = 11939, /* register          */
	POPCHK  = 47656, /* populate-chksum   */
	POPREM  = 65161, /* populate-remote   */
	SDESC   = 56620, /* show-description  */
	SFILES  = 47015, /* show-files        */
	SLIC    = 62833, /* show-license      */
	SMDEPS  = 64865, /* show-mdeps        */
	SNAME   = 5979,  /* show-name         */
	SRDEPS  = 29414, /* show-rdeps        */
	SSIZE   = 31953, /* show-size         */
	SVER    = 36360, /* show-version      */
	UNREG   = 37948, /* unregister        */
};

enum RTypes {
	LOCAL  = 1,
	REMOTE = 2,
	NONE   = 3
};

static void
pnode(Membuf mp)
{
	char *p;
	p = mp.p;
	for (;;) {
		p += printf("%s", p) + 1;
		if (p - mp.p >= mp.n) break;
		putchar(' ');
	}
}

/* action functions */
static int
add(Package *pkg)
{
	Membuf p1, p2;
	int rval;
	char *p;

	rval = 0;

	membuf_strinit(&p1);
	membuf_vstrcat(&p1, PKG_TMP, pkg->name.p, "#", pkg->version.p, "/");
	while ((p = db_walkfile(pkg))) {
		p1.n -= membuf_strcat(&p1, p);
		membuf_strinit(&p2);
		membuf_vstrcat(&p2, PKG_DIR, p);
		if (move(p1.p, p2.p) < 0) rval = 1;
		membuf_free(&p2);
	}

	membuf_free(&p1);
	membuf_free(&p2);

	return rval;
}

static int
del(Package *pkg)
{
	Membuf mp;
	int rval;
	char *p;

	rval = 0;

	membuf_strinit(&mp);
	membuf_strcat(&mp, PKG_DIR);
	while ((p = db_walkfile(pkg))) {
		mp.n -= membuf_strcat(&mp, p);
		if (remove(mp.p) < 0) rval = 1;
	}

	membuf_free(&mp);

	return rval;
}

static int
explode(Package *pkg)
{
	Membuf mp;
	int fd[2], rval;

	fd[0] = fd[1] = -1;
	rval  = 0;

	membuf_strinit(&mp);
	membuf_vstrcat(&mp, PKG_TMP, pkg->name.p, "#", pkg->version.p);

	if (mkdir(mp.p, ACCESSPERMS) < 0) {
		warn("mkdir %s", mp.p);
		goto failure;
	}

	/* unarchive acts locally */
	if (chdir(mp.p) < 0) {
		warn("chdir %s", mp.p);
		goto failure;
	}

	membuf_strcat(&mp, PKG_FMT);

	if ((fd[0] = open(mp.p, O_RDONLY, 0)) < 0) {
		warn("open %s", mp.p);
		goto failure;
	}
	membuf_free(&mp);

	membuf_strinit(&mp);
	membuf_vstrcat(&mp, PKG_TMP, pkg->name.p, "#", pkg->version.p, ".ust");

	if ((fd[1] = open(mp.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", mp.p);
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
	membuf_free(&mp);
	return rval;
}

static int
fetch(Package *pkg)
{
	Membuf mp;
	size_t siz;
	int fd, rval;
	unsigned sum;

	fd = -1;
	rval  = 0;

	membuf_strinit(&mp);
	membuf_vstrcat(&mp, PKG_TMP, pkg->name.p, "#", pkg->version.p, PKG_FMT);
	if ((fd = open(mp.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", mp.p);
		goto failure;
	}
	membuf_free(&mp);

	membuf_strinit(&mp);
	membuf_vstrcat(&mp, PKG_SRC, pkg->name.p, "#", pkg->version.p, PKG_FMT);
	if (netfd(mp.p, fd, NULL) < 0)
		goto failure;
	membuf_free(&mp);

	lseek(fd, 0, SEEK_SET);

	siz = 0;
	sum = filetosum(fd, &siz);

	if (chksum(pkg, siz, sum) < 0)
		rval = 1;

	goto done;
failure:
	rval = 1;
done:
	if (fd != -1)
		close(fd);
	membuf_free(&mp);
	return rval;
}

static int
regpkg(Package *pkg)
{
	Membuf p;

	membuf_strinit(&p);
	membuf_vstrcat(&p, GETDB(LOCAL), pkg->name.p);

	if (copy(pkg->path.p, p.p) < 0)
		return 1;

	membuf_free(&p);

	return 0;
}

static int
show_desc(Package *pkg)
{
	if (*pkg->description.p) puts(pkg->description.p);
	return 0;
}

static int
show_files(Package *pkg)
{
	char *p;
	if (!(p = db_walkfile(pkg))) return 0;
	printf("%s%s", PKG_DIR, p);
	while ((p = db_walkfile(pkg))) printf(" %s%s", PKG_DIR, p);
	return 0;
}

static int
show_lic(Package *pkg)
{
	if (*pkg->license.p) puts(pkg->license.p);
	return 0;
}

static int
show_mdeps(Package *pkg)
{
	pnode(pkg->mdeps);
	return 0;
}

static int
show_name(Package *pkg)
{
	if (*pkg->name.p) puts(pkg->name.p);
	return 0;
}

static int
show_rdeps(Package *pkg)
{
	pnode(pkg->rdeps);
	return 0;
}

static int
show_size(Package *pkg)
{
	printf("%zu\n", pkg->size);
	return 0;
}

static int
show_ver(Package *pkg)
{
	if (*pkg->version.p) puts(pkg->version.p);
	return 0;
}

static int
unregpkg(Package *pkg)
{
	return remove(pkg->path.p) < 0;
}

static int
populate_chksum(void)
{
	Membuf p;
	int fd, rval;

	membuf_strinit(&p);
	membuf_strcat(&p, PKG_CHK);

	fd = -1;
	rval = 0;

	if ((fd = open(p.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p.p);
		goto failure;
	}
	membuf_free(&p);

	membuf_strinit(&p);
	membuf_vstrcat(&p, PKG_SDB, PKG_FSG);
	if (netfd(p.p, fd, NULL) < 0) {
		close(fd);
		goto failure;
	}
	membuf_free(&p);

	goto done;;
failure:
	rval = 1;
done:
	if (fd != -1)
		close(fd);
	membuf_free(&p);
	return rval;
}

static int
populate_remote(void)
{
	Membuf p;
	int fd[2], rval;

	fd[0] = fd[1] = -1;
	rval  = 0;

	membuf_strinit(&p);
	membuf_vstrcat(&p, PKG_TMP, PKG_FDB);

	if ((fd[0] = open(p.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p.p);
		goto failure;
	}
	membuf_free(&p);

	membuf_strinit(&p);
	membuf_vstrcat(&p, PKG_SDB, PKG_FDB);

	if (netfd(p.p, fd[0], NULL) < 0)
		goto failure;
	membuf_free(&p);

	membuf_strinit(&p);
	membuf_vstrcat(&p, PKG_TMP, PKG_FDB, ".ustar");

	if ((fd[1] = open(p.p, MODERWCT, DEFFILEMODE)) < 0) {
		warn("open %s", p.p);
		goto failure;
	}
	membuf_free(&p);

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
	membuf_free(&p);
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
	Package pkg = { 0 };
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
	case POPCHK:
		exit(populate_chksum());
	case POPREM:
		exit(populate_remote());
	case SDESC:
		fn   = show_desc;
		type = LOCAL;
		break;
	case SFILES:
		fn   = show_files;
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
	case SSIZE:
		fn   = show_size;
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
	default:
		usage();
	}

	argc--, argv++;

	membuf_strinit(&p);
	membuf_strcat(&p, GETDB((atype == 0) ? type : atype));
	for (; *argv; argc--, argv++) {
		p.n -= membuf_strcat(&p, *argv);
		if (!(db_open(&pkg, p.p))) {
			if (errno == ENOMEM) exit(1);
			rval = 1;
			continue;
		}
		rval |= fn(&pkg);
	}

	return rval;
}
