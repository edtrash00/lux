#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

static int dflag;
static int lflag;
static int rflag;

static void
print_node(const char prefix, struct node *np)
{
	char path[PATH_MAX], *str;

	for (; np; np = np->next) {
		str = (char *)np->data;

		/* dependencies have no path */
		if (prefix == 'R' || prefix == 'M') {
			printf("%c: %s\n", prefix, str);
			return;
		}

		if (*PKG_DIR == '/')
			snprintf(path, sizeof(path), "%s%s", PKG_DIR, str);
		else
			snprintf(path, sizeof(path), "%s/%s", PKG_DIR, str);

		printf("%c: %s\n", prefix, path);
	}
}

static int
info(Package *pkg)
{
	if (dflag || (!dflag && !lflag && !rflag))
		printf(
		    "Name:        %s\n"
		    "Version:     %s\n"
		    "License:     %s\n"
		    "Description: %s\n",
		    pkg->name, pkg->version, pkg->license, pkg->description);

	if (rflag) {
		if (dflag)
			puts("\nDependencies");
		print_node('R', pkg->rdeps);
		print_node('M', pkg->mdeps);
	}

	if (lflag) {
		if (dflag)
			puts("\nFiles:");
		print_node('D', pkg->dirs);
		print_node('F', pkg->files);
	}

	return 0;
}

int
info_main(int argc, char *argv[])
{
	int rval = 0;
	Package *pkg;

	ARGBEGIN {
	case 'd':
		dflag = 1;
		break;
	case 'l':
		lflag = 1;
		break;
	case 'r':
		rflag = 1;
		break;
	default:
		usage();
	} ARGEND

	for (; *argv; argc--, argv++) {
		if (db_eopen(*argv, &pkg)) {
			rval = 1;
			continue;
		}
		rval |= info(pkg);
		db_close(pkg);
	}

	return rval;
}
