#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "lux.h"

enum Flags {
	LFLAG = 0x04, /* list files */
	RFLAG = 0x20  /* list deps  */
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
info(Package *pkg, int opts)
{
	printf(
	    "Name:        %s\n"
	    "Version:     %s\n"
	    "License:     %s\n"
	    "Description: %s\n",
	    pkg->longname, pkg->version, pkg->license, pkg->description);

	if (opts & RFLAG) {
		print_node("R: ", pkg->rdeps, 1);
		print_node("M: ", pkg->mdeps, 1);
	}
	if (opts & LFLAG) {
		print_node("D: ", pkg->dirs,  0);
		print_node("F: ", pkg->files, 0);
	}

	if (opts)
		putchar('\n');
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s info [-lRr] package ...\n", getprogname());
	exit(1);
}

int
info_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int opts = 0, type = LOCAL, rval = 0;
	Package *pkg;

	ARGBEGIN {
	case 'l':
		opts |= LFLAG;
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
