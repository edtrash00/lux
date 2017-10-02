#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

/* number plus colon */
#define PREFIX (sizeof(char) * 2)

enum Type {
	Name        = '0',
	Version     = '1',
	License     = '2',
	Description = '3',
	RDependency = '4',
	MDependency = '5',
	Directory   = '6',
	File        = '7'
};

Package *
db_open(const char *file)
{
	char **sp, *buf = NULL;
	FILE *fp;
	Package *pkg;
	size_t size = 0;
	ssize_t len;
	struct node **np;

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

	while ((len = getline(&buf, &size, fp)) != EOF) {
		buf[len - 1] = '\0'; /* remove trailing newline */
		sp = NULL;
		np = NULL;

		switch (*buf) {
		case Name:
			sp = &pkg->name;
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
		}

		if (sp && !(*sp = strdup(buf + PREFIX)))
			goto err;
		if (np && pushnode(np, addelement(buf + PREFIX)) < 0)
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

	free(pkg);
}
