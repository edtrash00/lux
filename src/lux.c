#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "lux.h"

int
db_eopen(const char *path, Package **pkg)
{
	if (!(*pkg = db_open(path))) {
		if (errno == ENOMEM)
			err(1, NULL);
		warn("open_db %s", path);
		return 1;
	}

	return 0;
}

void
usage(void)
{
	fprintf(stderr,
	    "usage: %s add   [-L] package ...\n"
	    "       %s del   package ...\n"
	    "       %s fetch package ...\n"
	    "       %s info  [-dlrR] package ...\n",
	    getprogname(), getprogname(), getprogname(), getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	int (*fn)(int, char **) = NULL;

	setprogname(argv[0]);
	argc--, argv++;

	if (!argc)
		usage();

	switch (hash(*argv)) {
	case ADD:
		fn = add_main;
		break;
	case DEL:
		fn = del_main;
		break;
	case FETCH:
		fn = fetch_main;
		break;
	case INFO:
		fn = info_main;
		break;
	default:
		usage();
	}

	exit(fn(argc, argv));
}
