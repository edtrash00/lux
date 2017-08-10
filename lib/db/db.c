/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

/* number plus colon */
#define PREFIX (sizeof(char) * 2)

enum Type {
	Name = '0',
	Version = '1',
	License = '2',
	Description = '3',
	RDependency = '4',
	MDependency = '5',
	Directory = '6',
	File = '7'
};

static char *
estrdup(const char *s)
{
	char *new;
	if (!(new = strdup(s)))
		err(1, "strdup");
	return new;
}

Package *
open_db(const char *file)
{
	char *buf = NULL;
	FILE *fp;
	Package *pkg;
	size_t size = 0;
	ssize_t len;

	if (!(fp = fopen(file, "r")))
		return NULL;

	if (!(pkg = malloc(1 * sizeof(*pkg))))
		err(1, "malloc");

	pkg->rdeps = NULL;
	pkg->mdeps = NULL;
	pkg->dirs  = NULL;
	pkg->files = NULL;

	while ((len = getline(&buf, &size, fp)) != EOF) {
		buf[len - 1] = '\0';
		switch (*buf) {
		case Name:
			pkg->name = estrdup(buf + PREFIX);
			break;
		case Version:
			pkg->version = estrdup(buf + PREFIX);
			break;
		case License:
			pkg->license = estrdup(buf + PREFIX);
			break;
		case Description:
			pkg->description = estrdup(buf + PREFIX);
			break;
		case RDependency:
			pushnode(&pkg->rdeps, addelement(buf + PREFIX));
			break;
		case MDependency:
			pushnode(&pkg->mdeps, addelement(buf + PREFIX));
			break;
		case Directory:
			pushnode(&pkg->dirs,  addelement(buf + PREFIX));
			break;
		case File:
			pushnode(&pkg->files, addelement(buf + PREFIX));
			break;
		}
	}

	free(buf);
	fclose(fp);

	return pkg;
}

void
close_db(Package *pkg) {
	free(pkg->name);
	free(pkg->version);
	free(pkg->license);
	free(pkg->description);

	while (pkg->rdeps)
		freenode(popnode(&pkg->rdeps));
	while(pkg->mdeps)
		freenode(popnode(&pkg->mdeps));
	while(pkg->dirs)
		freenode(popnode(&pkg->dirs));
	while(pkg->files)
		freenode(popnode(&pkg->files));

	free(pkg);
}
