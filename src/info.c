#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "pkg.h"

enum Flags {
	AFLAG = 0x01, /* print about      */
	DFLAG = 0x02, /* list directories */
	FFLAG = 0x04, /* list files       */
	MFLAG = 0x08, /* list run deps    */
	PFLAG = 0x10, /* print prefix     */
	RFLAG = 0x20, /* list make deps   */
	PUTCH = 0x40  /* print space      */
};

static int
print_node(const char *prefix, struct node *np, int opts)
{
	char path[PATH_MAX], *str;

	for (; np; np = np->next) {
		str = np->data;
		if (opts & PUTCH)
			putchar((opts & PFLAG) ? '\n' : ' ');

		if (opts & PFLAG)
			printf("%s", prefix);

		if (*prefix == 'D' || *prefix == 'F')
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
		else
			snprintf(path, sizeof(path), "%s", str);

		printf("%s", path);
	}

	return (PUTCH);
}

static void
info(Package *pkg, int opts)
{
	if (opts & AFLAG)
		printf(
		    "Name:        %s\n"
		    "Version:     %s\n"
		    "License:     %s\n"
		    "Description: %s\n",
		    pkg->name, pkg->version, pkg->license, pkg->description);

	if (opts & RFLAG)
		opts |= print_node("R: ", pkg->rdeps, opts);
	if (opts & MFLAG)
		opts |= print_node("M: ", pkg->mdeps, opts);
	if (opts & DFLAG)
		opts |= print_node("D: ", pkg->dirs,  opts);
	if (opts & FFLAG)
		opts |= print_node("F: ", pkg->files, opts);

	if ((opts & ~AFLAG))
		putchar('\n');
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s info [-R] [-adfmpr] package ...\n",
	    getprogname());
	exit(1);
}

int
info_main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	int opts = 0, type = LOCAL, rval = 0;
	Package *pkg;

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
