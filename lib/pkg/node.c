#include <stdlib.h>
#include <string.h>

#include "pkg.h"

Node *
addelement(char *s, Membuf *p)
{
	Node *new;

	new   = (Node *)(p->p + p->n);
	p->n += sizeof(*new);

	new->data = p->p + p->n;
	if (membuf_dstrcat(p, s) < 0)
		goto failure;

	goto done;
failure:
	new = NULL;
done:
	return new;
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
