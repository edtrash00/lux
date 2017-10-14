#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

enum Flags {
	DFLAG = 0x01, /* description */
	HFLAG = 0x02, /* list files */
	LFLAG = 0x04, /* list run deps  */
	MFLAG = 0x08, /* list make deps */
	NFLAG = 0x10, /* emits break line */
	RFLAG = 0x20  /* human readable output */
};

static int putch;

static void
pkey(const char *prefix, const char *str)
{
	char brk, *pre;

	pre = prefix ? (char *)prefix : "";
	brk = prefix ? '\n' : ' ';

	if (putch++)
		putchar(brk);

	printf("%s%s", pre, str);
}

static void
print_node(const char *prefix, struct node *np, int dep)
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

		if (dep)
			printf("%s%s", pre, str);
		else
			printf("%s%s", pre, path);
	}
}

static int
info(Package *pkg, int opts)
{
	if (opts & DFLAG) {
		pkey(opts & HFLAG ? "Name: " : NULL, pkg->name);
		pkey(opts & HFLAG ? "Version: " : NULL, pkg->version);
		pkey(opts & HFLAG ? "License: " : NULL, pkg->license);
		pkey(opts & HFLAG ? "Description: ": NULL, pkg->description);
	}

	if (opts & RFLAG)
		print_node(opts & HFLAG ? "R: " : NULL, pkg->rdeps, 1);
	if (opts & MFLAG)
		print_node(opts & HFLAG ? "M: " : NULL, pkg->mdeps, 1);
	if (opts & LFLAG) {
		if (opts & (RFLAG | MFLAG))
			putchar(opts & HFLAG ? '\n' : ' ');
		print_node(opts & HFLAG ? "D: " : NULL, pkg->dirs,  0);
		print_node(opts & HFLAG ? "F: " : NULL, pkg->files, 0);
	}

	if (-opts & NFLAG)
		putchar('\n');

	return 0;
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

	if (!opts)
		opts |= DFLAG|HFLAG;

	for (; *argv; argc--, argv++) {
		snprintf(buf, sizeof(buf), "%s/%s", GETDB(type), *argv);
		if (db_eopen(buf, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= info(pkg, opts);
		db_close(pkg);
	}

	return rval;
}
