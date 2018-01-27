#include <stdio.h>
#include <stdlib.h>

#include "pkg.h"

enum {
	ADD   = 97,  /* add   */
	DEL   = 109, /* del   */
	FETCH = 124, /* fetch */
	INFO  = 14   /* info  */
};

static void
usage(void)
{
	fprintf(stderr, "usage: %s add|del|fetch|info [args] ...\n",
	    getprogname());
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

	switch (strtohash(*argv)) {
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
