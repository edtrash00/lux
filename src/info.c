#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "pkg.h"

enum {
	LFLAG = 0x01, /* list files */
	RFLAG = 0x02, /* list deps  */
};

static int
print_node(const char *prefix, struct node *np, int putch)
{
	char path[PATH_MAX], *str;

	for (; np; np = np->next) {
		str = np->data;
		if (putch)
			putchar(' ');

		if (*prefix == 'D' || *prefix == 'F') {
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
			printf("%s%s", prefix, path);
		} else {
			printf("%s%s", prefix, str);
		}
	}

	return 1;
}

static void
info(Package *pkg, int opts)
{
	int putch = 0;

	printf(
	    "Name:        %s\n"
	    "Version:     %s\n"
	    "License:     %s\n"
	    "Description: %s\n",
	    pkg->name, pkg->version, pkg->license, pkg->description);

	if (opts & RFLAG) {
		putch = print_node("R: ", pkg->rdeps, putch);
		putch = print_node("M: ", pkg->mdeps, putch);
	}
	if (opts & LFLAG) {
		putch = print_node("D: ", pkg->dirs,  putch);
		putch = print_node("F: ", pkg->files, putch);
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
