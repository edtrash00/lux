#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "pkg.h"

void
membuf_strinit(Membuf *p)
{
	p->a = 0;
	p->n = 0;
	p->p = ialloc();
}

static ssize_t
membuf_strcats(Membuf *p, char *s, size_t n)
{
	if (p->n + n >= p->a) {
		p->p = ialloc_re(p->p, p->n, p->n + n + 1);
		p->a += n + 1;
	}

	memmove(p->p + p->n, s, n);
	p->n += n;
	p->p[p->n] = '\0';

	return n;
}

ssize_t
membuf_strcat(Membuf *p, char *s)
{
	return membuf_strcats(p, s, strlen(s));
}

ssize_t
membuf_vstrcat_(Membuf *p, char *s0, ...)
{
	va_list ap;
	ssize_t r, t;
	char *s;

	t = 0;
	s = s0;

	va_start(ap, s0);
	while (s) {
		if ((r = membuf_strcat(p, s)) < 0) {
			va_end(ap);
			return -1;
		}
		t += r;
		s  = va_arg(ap, char *);
	}
	va_end(ap);

	return t;
}

void
membuf_free(Membuf *p)
{
	ialloc_free(p->p, p->a);
	memset(p, 0, sizeof(*p));
}
