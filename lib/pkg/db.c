#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

#define init1(a, b, c) \
membuf_strinit_(&(a), (b), sizeof((b)));\
membuf_strcat(&(a), (c))

#define init2(a, b)\
membuf_dstrcat(&(a), (b));\
membuf_dstrcat(&(a), "\0");\
(a).n++

enum {
	NAME        = 31371, /* name        */
	VERSION     = 7384,  /* version     */
	LICENSE     = 33857, /* license     */
	DESCRIPTION = 33788, /* description */
	SIZE        = 57345, /* size        */
	RUNDEP      = 30157, /* run-dep     */
	MAKEDEP     = 22448, /* make-dep    */
	DIRECTORY   = 33933, /* dir         */
	AFILE       = 62844, /* file        */
	FLAG        = 65388  /* flag        */
};

static void
db_clean(Package *pkg)
{
	(pkg->dirs).n  = 0;
	(pkg->files).n = 0;
	(pkg->flags).n = 0;
	(pkg->mdeps).n = 0;
	(pkg->rdeps).n = 0;
	if ((pkg->dirs).p)
		memset((pkg->dirs).p,  0, 1);
	if ((pkg->files).p)
		memset((pkg->files).p, 0, 1);
	if ((pkg->flags).p)
		memset((pkg->flags).p, 0, 1);
	if ((pkg->mdeps).p)
		memset((pkg->mdeps).p, 0, 1);
	if ((pkg->rdeps).p)
		memset((pkg->rdeps).p, 0, 1);
}

void
db_init(Package *pkg)
{
	memset(pkg, 0, sizeof(*pkg));
	membuf_strinit_(&pkg->dirs,  NULL, PKG_VARSIZE);
	membuf_strinit_(&pkg->files, NULL, PKG_VARSIZE);
	membuf_strinit_(&pkg->flags, NULL, PKG_VARSIZE);
	membuf_strinit_(&pkg->mdeps, NULL, PKG_VARSIZE);
	membuf_strinit_(&pkg->rdeps, NULL, PKG_VARSIZE);
}

Package *
db_open(Package *pkg, char *file)
{
	FILE *fp;
	Membuf mp;
	ssize_t len;
	char buf[LINE_MAX];
	char *p;

	if (!(fp = fopen(file, "r"))) {
		warn("fopen %s", file);
		goto failure;
	}

	db_clean(pkg);
	membuf_strinit_(&mp, pkg->path, sizeof(pkg->path));
	membuf_strcat(&mp, file);

	while ((len = fgetline(buf, sizeof(buf), fp)) != EOF) {
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
			init1(mp, pkg->name, p);
			break;
		case VERSION:
			init1(mp, pkg->version, p);
			break;
		case LICENSE:
			init1(mp, pkg->license, p);
			break;
		case DESCRIPTION:
			init1(mp, pkg->description, p);
			break;
		case SIZE:
			pkg->size = strtobase(p, 0, UINT_MAX, 10);
			break;
		case RUNDEP:
			init2(pkg->rdeps, p);
			break;
		case MAKEDEP:
			init2(pkg->mdeps, p);
			break;
		case DIRECTORY:
			init2(pkg->dirs, p);
			break;
		case AFILE:
			init2(pkg->files, p);
			break;
		case FLAG:
			init2(pkg->flags, p);
			break;
		default:
			continue;
		}
	}

	goto done;
failure:
	pkg = NULL;
done:
	if (fp)
		fclose(fp);

	return pkg;
}

void
db_free(Package *pkg)
{
	free(pkg->dirs.p);
	free(pkg->files.p);
	free(pkg->flags.p);
	free(pkg->mdeps.p);
	free(pkg->rdeps.p);
}
