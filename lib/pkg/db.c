#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

#define init1(a, b) \
{ if ((a).n) continue; membuf_strinit(&(a)); membuf_strcat(&(a), (b)); }

#define init2(a, b) \
{ membuf_strcat(&(a), (b)); (a).n++; }

enum {
	NAME        = 31371, /* name        */
	VERSION     = 7384,  /* version     */
	LICENSE     = 33857, /* license     */
	DESCRIPTION = 33788, /* description */
	SIZE        = 57345, /* size        */
	RUNDEP      = 30157, /* run-dep     */
	MAKEDEP     = 22448, /* make-dep    */
	AFILE       = 62844, /* file        */
};

static void
db_clean(Package *pkg)
{
	membuf_free(&pkg->name);
	membuf_free(&pkg->version);
	membuf_free(&pkg->license);
	membuf_free(&pkg->description);
	membuf_free(&pkg->path);
	membuf_free(&pkg->mdeps);
	membuf_free(&pkg->rdeps);
}

Package *
db_open(Package *pkg, char *file)
{
	ssize_t len;
	char buf[LINE_MAX];
	char *p;

	if (pkg->fp)
		fclose(pkg->fp);

	if (!(pkg->fp = fopen(file, "r"))) {
		warn("fopen %s", file);
		goto failure;
	}

	db_clean(pkg);
	do { init1(pkg->path, file) } while (0);

	while ((len = fgetline(buf, sizeof(buf), pkg->fp)) != EOF) {
		buf[len-1] = '\0'; /* remove trailing newline */

		/* ignore blank lines */
		if (*buf == '\0' || *buf == '#')
			continue;

		if ((p = strchr(buf, ':')))
			*p++ = '\0';

		if (!p || !(*p))
			continue;

		switch (strtohash(buf)) {
		case NAME:
			init1(pkg->name, p);
			break;
		case VERSION:
			init1(pkg->version, p);
			break;
		case LICENSE:
			init1(pkg->license, p);
			break;
		case DESCRIPTION:
			init1(pkg->description, p);
			break;
		case SIZE:
			pkg->size = strtobase(p, 0, LLONG_MAX, 10);
			break;
		case RUNDEP:
			init2(pkg->rdeps, p);
			break;
		case MAKEDEP:
			init2(pkg->mdeps, p);
			break;
		case AFILE:
			goto done;
		default:
			continue;
		}
	}

	goto done;
failure:
	pkg = NULL;
done:
	return pkg;
}

char *
db_walkfile(Package *pkg)
{
	ssize_t len;
	char buf[LINE_MAX];
	char *p;

	while ((len = fgetline(buf, sizeof(buf), pkg->fp)) != EOF) {
		buf[len-1] = '\0';

		/* ignore blank lines */
		if (*buf == '\0' || *buf == '#')
			continue;

		if ((p = strchr(buf, ':')))
			*p++ = '\0';

		if (!p || !(*p))
			continue;

		switch (strtohash(buf)) {
		case AFILE:
			return p;
		default:
			warnx("bad formed database file");
			continue;
		}
	}

	return NULL;
}
