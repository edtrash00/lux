#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "lux.h"

enum Flags {
	DFLAG = 0x01, /* description */
	HFLAG = 0x02, /* human readable output */
	LFLAG = 0x04, /* list files */
	MFLAG = 0x08, /* list make deps */
	NFLAG = 0x10, /* emits break line */
	RFLAG = 0x20  /* list run deps  */
};

static int putch;

static void
print_node(const char *prefix, struct node *np, int nopath)
{
	char path[PATH_MAX];
	char brk, *pre, *str;

	for (; np; np = np->next) {
		str = (char *)np->data;
		pre = prefix ? (char *)prefix : "";
		brk = prefix ? '\n' : ' ';

		if (putch++)
			putchar(brk);

		if (*PKG_DIR == '/')
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
		else
			snprintf(path, sizeof(path), "%s/%s", PKG_DIR, str);

		if (nopath)
			printf("%s%s", pre, str);
		else
			printf("%s%s", pre, path);
	}
}

static void
getinfo(Package *pkg, int opts)
{
	printf(
	    "Name:        %s\n"
	    "Version:     %s\n"
	    "License:     %s\n"
	    "Description: %s\n",
	    pkg->name, pkg->version, pkg->license, pkg->description);
}

static void
getargs(Package *pkg, int opts)
{
	const char *rdeps, *mdeps, *dirs, *files;

	if (opts & HFLAG) {
		rdeps       = "R: ";
		mdeps       = "M: ";
		dirs        = "D: ";
		files       = "F: ";
	} else {
		rdeps       = NULL;
		mdeps       = NULL;
		dirs        = NULL;
		files       = NULL;
	}

	if (opts & RFLAG)
		print_node(rdeps, pkg->rdeps, 1);
	if (opts & MFLAG)
		print_node(mdeps, pkg->mdeps, 1);
	if (opts & LFLAG) {
		print_node(dirs, pkg->dirs,  0);
		print_node(files, pkg->files, 0);
	}

	if (-opts & NFLAG)
		putchar('\n');
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s info [-Rd] package ...\n"
	    "       %s info [-Rh] [-lmnr] package ...\n",
	    getprogname(), getprogname());
	exit(1);
}

int
info_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int opts = 0, type = LOCAL, rval = 0;
	Package *pkg;

	ARGBEGIN {
	case 'd':
		opts |= DFLAG;
		break;
	case 'h':
		opts |= HFLAG;
		break;
	case 'l':
		opts |= LFLAG;
		break;
	case 'm':
		opts |= MFLAG;
		break;
	case 'n':
		opts |= NFLAG;
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
		opts = DFLAG;

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(type), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		if (opts & DFLAG)
			getinfo(pkg, opts);
		else
			getargs(pkg, opts);
		db_close(pkg);
	}

	return rval;
}
