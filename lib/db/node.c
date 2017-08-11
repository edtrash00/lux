/* This file is part of the PkgUtils from EltaninOS
 * See LICENSE file for copyright and license details.
 */
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"

struct node *
addelement(const void *value)
{
	struct node *new;

	if (!(new = malloc(1 * sizeof(*new))))
		goto failure;

	if (!(new->data = strdup(value)))
		goto failure;

	goto done;
failure:
	free(new);
	new = NULL;
done:
	return new;
}

void
freenode(struct node *old)
{
	free(old->data);
	free(old);
}

struct node *
popnode(struct node **sp)
{
	struct node *old;

	old = *sp;
	*sp = old->next;

	return old;
}

int
pushnode(struct node **sp, struct node *new)
{
	int rval = 0;

	if (new) {
		new->next = *sp;
		*sp = new;
	} else {
		rval = -1;
	}

	return rval;
}
