#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "pkg.h"

static int
mvtosys(const char *path)
{
	char buf[PATH_MAX];

	if (*PKG_DIR == '/')
		snprintf(buf, sizeof(buf), "%s%s", PKG_DIR, path);
	else
		snprintf(buf, sizeof(buf), "%s/%s", PKG_DIR, path);

	if (move(path, buf) < 0) {
		warn("move %s -> %s", path, buf);
		return 1;
	}

	return 0;
}

static int
add(Package *pkg)
{
	int rval = 0;
	struct node *np;

	for (np = pkg->dirs; np; np = np->next)
		rval |= mvtosys(np->data);
	for (np = pkg->files; np; np = np->next)
		rval |= mvtosys(np->data);

	return rval;
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s add [-L] package ...\n", getprogname());
	exit(1);
}

int
add_main(int argc, char *argv[])
{
	char buf[PATH_MAX], lp[PATH_MAX];
	int type = REMOTE, rval = 0;
	Package *pkg;

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
			rval = 1;\
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
