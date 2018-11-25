#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "pkg.h"

void
membuf_strinit(Membuf *p, char *s, size_t n)
{
	p->a = n;
	p->n = 0;
	p->p = s;
}

ssize_t
membuf_strcats(Membuf *p, char *s, size_t n)
{
	if (p->n + n >= p->a)
		return -1;

	memmove(p->p + p->n, s, n);
	p->n += n;
	p->p[p->n] = '\0';

	return n;
}

ssize_t
membuf_strcat(Membuf *p, char *s)
{
	return (membuf_strcats(p, s, strlen(s)));
}

ssize_t
membuf_vstrcat_(Membuf *p, char *s0, ...)
{
	va_list ap;
	ssize_t r, t;
	char   *s;

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

ssize_t
membuf_dstrcat(Membuf *p, char *s)
{
	ssize_t n;
	n = strlen(s);
	if (!(p->p)) {
		if (!(p->p = malloc(p->a)))
			return -1;
	}
	for (; p->n + n >= p->a;) {
		p->a *= 2;
		if (!(p->p = realloc(p->p, p->a)))
			return -1;
	}
	return membuf_strcats(p, s, n);
}
