#include <err.h>
#include <stdio.h>
#include <limits.h>

#include "lux.h"

enum Flags {
	DESC = 0x1, /* description */
	LIST = 0x2, /* list files */
	DEPS = 0x4  /* list deps  */
};

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
info(Package *pkg, int opts)
{
	if (opts & DESC || !opts)
		printf(
		    "Name:        %s\n"
		    "Version:     %s\n"
		    "License:     %s\n"
		    "Description: %s\n",
		    pkg->name, pkg->version, pkg->license, pkg->description);

	if (opts & DEPS) {
		if (opts & DESC)
			puts("\nDependencies");
		print_node('R', pkg->rdeps);
		print_node('M', pkg->mdeps);
	}

	if (opts & LIST) {
		if (opts & DESC)
			puts("\nFiles:");
		print_node('D', pkg->dirs);
		print_node('F', pkg->files);
	}

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
		opts |= DESC;
		break;
	case 'l':
		opts |= LIST;
		break;
	case 'r':
		opts |= DEPS;
		break;
	case 'R':
		type = REMOTE;
		break;
	default:
		usage();
	} ARGEND

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
