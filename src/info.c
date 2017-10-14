#include <err.h>
#include <stdio.h>
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
pkey(const char *prefix, const char *str)
{
	char brk, *pre;

	pre = prefix ? (char *)prefix : "";
	brk = prefix ? '\n' : ' ';

	if (putch++)
		putchar(brk);

	printf("%s%s", pre, (char *)str);
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
	const char *name, *version, *license, *description;
	const char *rdeps, *mdeps, *dirs, *files;

	if (opts & HFLAG) {
		name        = "Name: ";
		version     = "Version: ";
		license     = "License: ";
		description = "Description: ";
		rdeps       = "R: ";
		mdeps       = "M: ";
		dirs        = "D: ";
		files       = "F: ";
	} else {
		name        = NULL;
		version     = NULL;
		license     = NULL;
		description = NULL;
		rdeps       = NULL;
		mdeps       = NULL;
		dirs        = NULL;
		files       = NULL;
	}

	if (opts & DFLAG) {
		pkey(name, pkg->name);
		pkey(version, pkg->version);
		pkey(license, pkg->license);
		pkey(description, pkg->description);
	}

	if (opts & RFLAG)
		print_node(rdeps, pkg->rdeps, 1);
	if (opts & MFLAG)
		print_node(mdeps, pkg->mdeps, 1);
	if (opts & LFLAG) {
		if (opts & (RFLAG | MFLAG))
			putchar(opts & HFLAG ? '\n' : ' ');
		print_node(dirs, pkg->dirs,  0);
		print_node(files, pkg->files, 0);
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
		opts = DFLAG|HFLAG;

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
