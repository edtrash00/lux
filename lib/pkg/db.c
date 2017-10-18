#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

Package *
db_open(const char *file)
{
	char **sp, *buf = NULL, *p;
	FILE *fp;
	off_t *op;
	Package *pkg;
	size_t size = 0;
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
	pkg->longdesc    = NULL;
	pkg->rdeps       = NULL;
	pkg->mdeps       = NULL;
	pkg->dirs        = NULL;
	pkg->files       = NULL;
	pkg->flags       = NULL;

	while ((len = getline(&buf, &size, fp)) != EOF) {
		buf[len - 1] = '\0'; /* remove trailing newline */
		sp = NULL;
		np = NULL;
		op = NULL;

		/* ignore blank lines */
		if (buf == NULL || *buf == '\0')
			continue;

		for (p = buf; *p != ':'; p++)
			continue;
		*p++ = '\0';

		switch (hash(buf)) {
		case Name:
			sp = &pkg->name;
			break;
		case LongName:
			sp = &pkg->longname;
			break;
		case Version:
			sp = &pkg->version;
			break;
		case License:
			sp = &pkg->license;
			break;
		case Description:
			sp = &pkg->description;
			break;
		case LongDesc:
			np = &pkg->longdesc;
			break;
		case ByteSize:
			op = &pkg->size;
			break;
		case BytePkgSize:
			op = &pkg->pkgsize;
			break;
		case RDependency:
			np = &pkg->rdeps;
			break;
		case MDependency:
			np = &pkg->mdeps;
			break;
		case Directory:
			np = &pkg->dirs;
			break;
		case File:
			np = &pkg->files;
			break;
		case Flag:
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
	free(buf);

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

	while (pkg->longdesc)
		freenode(popnode(&pkg->longdesc));
	while (pkg->rdeps)
		freenode(popnode(&pkg->rdeps));
	while (pkg->mdeps)
		freenode(popnode(&pkg->mdeps));
	while (pkg->dirs)
		freenode(popnode(&pkg->dirs));
	while (pkg->files)
		freenode(popnode(&pkg->files));

	free(pkg);
}
