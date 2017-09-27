#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lux.h"

enum Fn {
	ADD   = 0,
	DEL   = 1,
	FETCH = 2,
	INFO  = 3
};

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
	int fn, rval = 0;
	Package *pkg;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	if (!strcmp(*argv, "add"))
		fn = ADD;
	else if (!strcmp(*argv, "del"))
		fn = DEL;
	else if (!strcmp(*argv, "fetch"))
		fn = FETCH;
	else if (!strcmp(*argv, "info"))
		fn = INFO;
	else
		usage();

	argc--, argv++; 

	for (; *argv; argc--, argv++) {
		if (warn_open_db(&pkg, *argv)) {
			rval = 1;
			continue;
		}

		switch (fn) {
		case ADD:
			add(pkg, *argv);
			break;
		case DEL:
			del(pkg, *argv);
			break;
		case FETCH:
			fetch(pkg);
			break;
		case INFO:
			info(pkg);
			break;
		}

		close_db(pkg);
	}

	return rval;
}
