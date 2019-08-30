#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "pkg.h"

#define ALIGN(x) (((x)/16) * sizeof(aligned))

#define ISSTACK(x) \
((char *)(x) >= (char *)stackpool && (char *)(x) <= (char *)stackpool + POOLSIZE)
#define ISPOOL(x) \
((char *)(x) >= mp.p && (char *)(x) <= mp.p + mp.a)
#define ISMINE(x) \
(ISSTACK(x) || ISPOOL(x))
#define ISSALLOC(x) \
((x) == (void *)-1)

typedef union { char x[16]; double d; } aligned;
static aligned stackpool[POOLSIZE/16];
static Membuf mp = { POOLSIZE, 0, (char *)stackpool };
static size_t freesiz;
static int warned, warned2;

static int
ready(void)
{
	if (!ISSTACK(mp.p)) {
		if (!warned) {
			warnx("<warn> internal pools are full");
			++warned;
		}
		return 0;
	}
	freesiz = 0;
	if (!(mp.p = malloc(ALIGN(MPOOLSIZE)))) err(1, "malloc");
	mp.n = 0;
	mp.a = MPOOLSIZE;
	return 1;
}

void *
alloc(size_t n)
{
	void *p;
	n = (n + 16) - (n & (16 - 1));
	if (n > mp.a - mp.n) {
		ready();
		if ((n > mp.a - mp.n) && !ready()) return malloc(n);
	}
	p = mp.p + mp.n;
	mp.n += n;
	return p;
}

void
alloc_free(void *p, size_t n)
{
	if (!p) return;
	if (ISSALLOC(p) || ISPOOL(p)) {
		mp.n -= (n + 16) - (n & (16 - 1));
		return;
	}
	if (!ISSTACK(p)) free(p);
}

void *
alloc_re(void *p, size_t o, size_t n)
{
	void *x;
	if (ISMINE(p)) {
		if (!warned2) {
			warnx("<warn> non-optimal memory use");
			warned2++;
		}
		x = alloc(n);
		return memcpy(x, p, o);
	}
	return realloc(p, n);
}

/* incremental alloc */
void *
ialloc(void)
{
	return mp.p + mp.n;
}

void *
ialloc_re(void *p, size_t o, size_t n) {
	if (mp.n + n > mp.a) return alloc_re(p, o, n);
	mp.n += n - o;
	return p;
}

/* stupid alloc */
void *
salloc(size_t n)
{
	void *p;
	n = n + sizeof(size_t);
	n = (n + 16) - (n & (16 - 1));
	freesiz += n;
	p = alloc(n);
	memcpy(p, &n, sizeof(n));
	p = (char *)p + sizeof(size_t);
	return p;
}

void *
scalloc(size_t m, size_t n)
{
	size_t len;
	void *p;
	len = m*n;
	p = salloc(len);
	return memset(p, 0, len);
}

void
sfree(void *p)
{
	(void)p;
	return;
}

void *
srealloc(void *p, size_t n)
{
	size_t *len;
	if (!p) return salloc(n);
	len = (void *)((char *)p - sizeof(*len));
	freesiz += *len + sizeof(*len);
	return alloc_re(p, *len, n);
}

void
sfreeall(void)
{
	alloc_free((void *)-1, freesiz);
	freesiz = 0;
}
