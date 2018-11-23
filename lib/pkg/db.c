#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

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

Package *
db_open(Package *pkg, Membuf *mp, char *file)
{
	FILE *fp;
	Node **np;
	ssize_t len;
	char buf[LINE_MAX];
	char *p, **sp;

	if (!(fp = fopen(file, "r"))) {
		warn("fopen %s", file);
		goto failure;
	}

	pkg->path = mp->p + mp->n;
	if (membuf_dstrcat(mp, file) < 0) {
		warn("membuf_dstrcat");
		goto failure;
	}

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
		buf[len-1] = '\0'; /* remove trailing newline */
		sp = NULL;
		np = NULL;

		/* ignore blank lines */
		if (*buf == '\0' || *buf == '#')
			continue;

		if ((p = strchr(buf, ':')))
			*p++ = '\0';

		if (!p || !(*p))
			continue;

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
			pkg->size = strtobase(p, 0, UINT_MAX, 10);
			continue;
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
		default:
			continue;
		}

		if (sp) {
			*sp = mp->p + mp->n;
			if (membuf_dstrcat(mp, p) < 0) {
				warn("membuf_dstrcat");
				goto failure;
			}
			mp->n++;
		}

		if (np && pushnode(np, addelement(p, mp)) < 0) {
			warn("addelement %s", p);
			goto failure;
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
