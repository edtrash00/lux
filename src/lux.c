#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "pkg.h"

enum Hash {
	ADD     = 97,  /* add     */
	DEL     = 109, /* del     */
	EXPLODE = 111, /* explode */
	FETCH   = 124, /* fetch   */
	INFO    = 14   /* info    */
};

static int
db_eopen(const char *path, Package **pkg)
{
	if (!(*pkg = db_open(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, "add del explode fetch info\n");
	exit(1);
}

static int
add_main(int argc, char *argv[])
{
	Package *pkg;
	int type, rval;
	char buf[PATH_MAX], lp[PATH_MAX];

	type = REMOTE;
	rval = 0;

	ARGBEGIN {
	case 'L':
		type = NONE;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(type), *argv);
		snprintf(lp, sizeof(lp), "%s/%s", GETDB(LOCAL), *argv);

		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= add(pkg);
		db_close(pkg);

		if (copy(buf, lp) < 0) {
			warn("copy %s -> %s", buf, lp);
			rval = 1;
		}
	}

	return rval;
}

static int
del_main(int argc, char *argv[])
{
	Package *pkg;
	int rval;
	char buf[PATH_MAX];

	rval = 0;
	argc--, argv++; /* remove comand string */

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(LOCAL), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= del(pkg);
		db_close(pkg);

		if (remove(buf) < 0) {
			warn("remove %s", buf);
			rval = 1;
		}
	}

	return rval;
}

static int
explode_main(int argc, char *argv[])
{
	Package *pkg;
	int rval;
	char buf[PATH_MAX];

	rval = 0;
	argc--, argv++;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(REMOTE), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= explode(pkg);
		db_close(pkg);
	}

	return rval;
}

static int
fetch_main(int argc, char *argv[])
{
	Package *pkg;
	int rval;
	char buf[PATH_MAX];

	rval = 0;
	argc--, argv++; /* remove comand string */

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(REMOTE), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= fetch(pkg);
		db_close(pkg);
	}

	return rval;
}

static int
info_main(int argc, char *argv[])
{
	Package *pkg;
	int opts, type, rval;
	char buf[PATH_MAX];

	opts = 0;
	type = LOCAL;
	rval = 0;

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
	case 'R':
		type = REMOTE;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if (!opts)
		opts |= (AFLAG|DFLAG|FFLAG|MFLAG|PFLAG|RFLAG);

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(type), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		info(pkg, opts);
		db_close(pkg);
	}

	return rval;
}

int
main(int argc, char *argv[])
{
	int (*fn)(int, char **);

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	switch (strtohash(*argv)) {
	case ADD:
		fn = add_main;
		break;
	case DEL:
		fn = del_main;
		break;
	case EXPLODE:
		fn = explode_main;
		break;
	case FETCH:
		fn = fetch_main;
		break;
	case INFO:
		fn = info_main;
		break;
	default:
		usage();
	}

	exit(fn(argc, argv));
}
