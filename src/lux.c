#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lux.h"

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s add   package ...\n"
	    "       %s del   package ...\n"
	    "       %s fetch package ...\n"
	    "       %s info  package ...\n",
	    getprogname(), getprogname(), getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int (*fn)(const char *, Package *);
	int rval = 0;
	Package *pkg;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	if (!strcmp(*argv, "add"))
		fn = add;
	else if (!strcmp(*argv, "del"))
		fn = del;
	else if (!strcmp(*argv, "fetch"))
		fn = fetch;
	else if (!strcmp(*argv, "info"))
		fn = info;
	else
		usage();

	argc--, argv++; 

	for (; *argv; argc--, argv++) {
		if (warn_open_db(&pkg, *argv)) {
			rval = 1;
			continue;
		}
		rval |= fn(*argv, pkg);
		close_db(pkg);
	}

	return rval;
}
