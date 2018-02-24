#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

enum {
	NAME        = 7,   /* name        */
	VERSION     = 122, /* version     */
	LICENSE     = 123, /* license     */
	DESCRIPTION = 102, /* description */
	SIZE        = 5,   /* size        */
	PKGSIZE     = 121, /* pkgsize     */
	RUNDEP      = 53,  /* run-dep     */
	MAKEDEP     = 94,  /* make-dep    */
	DIRECTORY   = 127, /* dir         */
	AFILE       = 6,   /* file        */
	FLAG        = 12   /* flag        */
};

Package *
db_open(const char *file)
{
	FILE *fp;
	Package *pkg;
	struct node **np;
	ssize_t len;
	off_t *op;
	char **sp, *p, buf[LINE_MAX];

	if (!(fp = fopen(file, "r")))
		goto failure;

	if (!(pkg = malloc(1 * sizeof(*pkg))))
		goto failure;

	pkg->name        = NULL;
	pkg->version     = NULL;
	pkg->license     = NULL;
	pkg->description = NULL;
	pkg->rdeps       = NULL;
	pkg->mdeps       = NULL;
	pkg->dirs        = NULL;
	pkg->files       = NULL;
	pkg->flags       = NULL;

	while ((len = fgetline(buf, sizeof(buf), fp)) != EOF) {
		buf[len] = '\0'; /* remove trailing newline */
		sp = NULL;
		np = NULL;
		op = NULL;

		/* ignore blank lines */
		if (*buf == '\0' || *buf == '#')
			continue;

		if ((p = strchr(buf, ':')))
			*p++ = '\0';

		switch (strtohash(buf)) {
		case NAME:
			sp = &pkg->name;
			break;
		case VERSION:
			sp = &pkg->version;
			break;
		case LICENSE:
			sp = &pkg->license;
			break;
		case DESCRIPTION:
			sp = &pkg->description;
			break;
		case SIZE:
			op = &pkg->size;
			break;
		case PKGSIZE:
			op = &pkg->pkgsize;
			break;
		case RUNDEP:
			np = &pkg->rdeps;
			break;
		case MAKEDEP:
			np = &pkg->mdeps;
			break;
		case DIRECTORY:
			np = &pkg->dirs;
			break;
		case AFILE:
			np = &pkg->files;
			break;
		case FLAG:
			np = &pkg->flags;
			break;
		}

		if (op && (*op = stoll(p, 0, UINT_MAX, 10)) < 0)
			goto err;

		if (sp && !(*sp = strdup(p)))
			goto err;

		if (np && pushnode(np, addelement(p)) < 0)
			goto err;
	}

	goto done;
err:
	db_close(pkg);
failure:
	pkg = NULL;
done:
	if (fp)
		fclose(fp);

	return pkg;
}

void
db_close(Package *pkg) {
	free(pkg->name);
	free(pkg->version);
	free(pkg->license);
	free(pkg->description);

	while (pkg->rdeps)
		freenode(popnode(&pkg->rdeps));
	while (pkg->mdeps)
		freenode(popnode(&pkg->mdeps));
	while (pkg->dirs)
		freenode(popnode(&pkg->dirs));
	while (pkg->files)
		freenode(popnode(&pkg->files));
	while (pkg->flags)
		freenode(popnode(&pkg->flags));

	free(pkg);
}
