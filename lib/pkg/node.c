#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

Node *
addelement(const void *value)
{
	Node *new;

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
freenode(Node *old)
{
	free(old->data);
	free(old);
}

Node *
popnode(Node **sp)
{
	Node *old;

	old = *sp;
	*sp = old->next;

	return old;
}

int
pushnode(Node **sp, Node *new)
{
	if (!new)
		return -1;

	new->next = *sp;
	*sp = new;

	return 0;
}
