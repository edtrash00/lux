#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

#define NAME        6382843298            /* name        */
#define LONGNAME    249786571779427685    /* long-name   */
#define VERSION     229390708560831       /* version     */
#define LICENSE     229372341555262       /* license     */
#define DESCRIPTION 13751386334653244867U /* description */
#define SIZE        6383314208            /* size        */
#define PKGSIZE     229388381659868       /* pkgsize     */
#define RUNDEP      229394969610032       /* run-dep     */
#define MAKEDEP     7569334749620571      /* make-dep    */
#define DIRECTORY   193404666             /* dir         */
#define AFILE       6382564547            /* file        */
#define FLAG        6382568041            /* flag        */

Package *
db_open(const char *file)
{
	char buf[BUFSIZ], **sp, *p;
	FILE *fp;
	off_t *op;
	Package *pkg;
	ssize_t len;
	struct node **np;

	if (!(fp = fopen(file, "r")))
		goto failure;

	if (!(pkg = malloc(1 * sizeof(*pkg))))
		goto failure;

	pkg->name        = NULL;
	pkg->longname    = NULL;
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
		if (*buf == '\0')
			continue;

		for (p = buf; *p != ':'; p++)
			continue;
		*p++ = '\0';

		switch (hash(buf)) {
		case NAME:
			sp = &pkg->name;
			break;
		case LONGNAME:
			sp = &pkg->longname;
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

		if (op && (*op = stoll(p, 0, UINT_MAX)) < 0)
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
	free(pkg->longname);
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
