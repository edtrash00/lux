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
static int warned;

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
	memset(&freesiz, 0, sizeof(freesiz));
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
	if (ISSALLOC(p) || ISPOOL(p)) { mp.n -= n; return; }
	if (!p) return;
	if (!ISSTACK(p)) free(p);
}

void *
alloc_re(void *p, size_t o, size_t n)
{
	void *x;
	if (ISMINE(p)) {
		x = alloc(n);
		return memcpy(x, p, o);
	}
	return realloc(p, n);
}

/* stupid alloc */

void *
salloc(size_t n)
{
	size_t *len;
	len = alloc(sizeof(*len)); *len = n;
	return alloc(n);
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
	size_t *len;
	if (!p) return;
	if (!ISMINE(p)) { free(p); return; }
	len = (void *)((char *)p - sizeof(*len));
	freesiz += *len;
}

void *
srealloc(void *p, size_t n)
{
	size_t *len;
	if (!p) return alloc(n);
	if (!ISMINE(p)) return realloc(p, n);
	len = (void *)((char *)p - sizeof(*len));
	freesiz += *len;
	return alloc_re(p, *len, n);
}

void
sfreeall(void)
{
	alloc_free((void *)-1, freesiz);
	freesiz = 0;
}
