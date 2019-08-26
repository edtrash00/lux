#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"

int
copy(const char *src, const char *dest)
{
	ssize_t rf;
	int sf, tf, rval;
	char buf[BUFSIZ];

	sf   = tf = -1;
	rval =  0;

	if ((sf = open(src, O_RDONLY, 0)) < 0) {
		warn("open %s", src);
		goto failure;
	}

	if ((tf = open(dest, O_WRONLY | O_CREAT | O_EXCL, 644)) < 0) {
		warn("open %s", dest);
		goto failure;
	}

	while ((rf = read(sf, buf, sizeof(buf))) > 0) {
		if (write(tf, buf, rf) != rf) {
			warn("write %s", dest);
			goto failure;
		}
	}

	if (rf < 0) {
		warn("read %s", src);
		goto failure;
	}

	goto done;
failure:
	rval = -1;
done:
	if (sf != -1)
		close(sf);
	if (tf != -1)
		close(tf);

	return rval;
}

int
move(const char *src, const char *dest)
{
	struct stat st;
	size_t n;
	char *d;

	if (lstat(src, &st) < 0) {
		warn("lstat %s", src);
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		n = strlen(dest);
		d = alloc(n);
		memcpy(d, dest, n);
		if (mkdirp(dest, st.st_mode, ACCESSPERMS) < 0) {
			alloc_free(d, n);
			return -1;
		}
		alloc_free(d, n);
	} else {
		if (mkdirp(dircomp(dest), ACCESSPERMS, ACCESSPERMS) < 0)
			return -1;
		if (rename(src, dest) < 0) {
			warn("rename %s %s", src, dest);
			return -1;
		}
	}

	return 0;
}

int
remove(const char *path) {
	struct stat st;
	int (*fn)(const char *);

	if (lstat(path, &st) < 0) {
		warn("stat %s", path);
		return -1;
	}

	fn = S_ISDIR(st.st_mode) ? rmdir : unlink;

	if (fn(path) < 0) {
		warn("remove %s", path);
		return -1;
	}

	return 0;
}

int
mkdirp(char *path, mode_t dmode, mode_t mode)
{
	char *p, c;

	c = 0;
	p = path;

	if ((path[0] == '.' || path[0] == '/') && path[1] == 0)
		return 0;

	for (; *p; *p = c) {
		p += strspn(p, "/");
		p += strcspn(p, "/");

		c  = *p;
		*p = '\0';

		if (mkdir(path, c ? dmode : mode) < 0 && errno != EEXIST) {
			warn("mkdir %s", path);
			return -1;
		}
	}

	return 0;
}
