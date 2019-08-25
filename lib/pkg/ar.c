#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"
#include "zlib.h"

#define CHOWN(a, b, c, d) \
((((a) == SYMTYPE) ? s_lchown : s_chown)((b), (c), (d)))
#define TYPE(x) \
(((x) == CHRTYPE) ? S_IFCHR : ((x) == BLKTYPE) ? S_IFBLK : S_IFIFO)

enum {
	AREGTYPE = '\0',
	REGTYPE  = '0',
	LNKTYPE  = '1',
	SYMTYPE  = '2',
	CHRTYPE  = '3',
	BLKTYPE  = '4',
	DIRTYPE  = '5',
	FIFOTYPE = '6',
	CONTTYPE = '7'
};

#define h_name(x)     ((x) +   0)
#define h_mode(x)     ((x) + 100)
#define h_uid(x)      ((x) + 108)
#define h_gid(x)      ((x) + 116)
#define h_size(x)     ((x) + 124)
#define h_mtime(x)    ((x) + 136)
#define h_chksum(x)   ((x) + 148)
#define h_type(x)     ((x) + 156)
#define h_linkname(x) ((x) + 157)
#define h_magic(x)    ((x) + 257)
#define h_version(x)  ((x) + 263)
#define h_uname(x)    ((x) + 265)
#define h_gname(x)    ((x) + 297)
#define h_major(x)    ((x) + 329)
#define h_minor(x)    ((x) + 337)
#define h_prefix(x)   ((x) + 345)

static void
sanitize(char *h)
{
	char *p;
	if ((p = memchr(h_mode(h),   ' ',  8))) memset(p, 0, h_gid(h)    - p);
	if ((p = memchr(h_gid(h),    ' ',  8))) memset(p, 0, h_size(h)   - p);
	if ((p = memchr(h_size(h),   ' ', 12))) memset(p, 0, h_mtime(h)  - p);
	if ((p = memchr(h_mtime(h),  ' ', 12))) memset(p, 0, h_chksum(h) - p);
	if ((p = memchr(h_chksum(h), ' ',  8))) memset(p, 0, h_major(h)  - p);
	if ((p = memchr(h_major(h),  ' ',  8))) memset(p, 0, h_minor(h)  - p);
	if ((p = memchr(h_minor(h),  ' ',  8))) memset(p, 0, h_prefix(h) - p);
}

int
unarchive(int tarfd)
{
	struct timespec tms[2];
	ssize_t r;
	long gid, maj, min, m, mtim, size, typ, uid;
	int fd;
	char buf[512], fname[101];

	for (;;) {
		if (!(r = read(tarfd, buf, sizeof(buf))))
			break;
		if (r < 0) {
			warn("read");
			return -1;
		}
		if (!(*h_name(buf)))
			break;

		sanitize((char *)buf);

		fname[0] = 0;
		if (*h_prefix(buf))
			strncat(fname, h_prefix(buf), 155);

		strncat(fname, h_name(buf), 100);

		m = strtomode(h_mode(buf), ACCESSPERMS);

		if (mkdirp(dircomp(fname), ACCESSPERMS, ACCESSPERMS) < 0)
			return -1;

		switch (*h_type(buf)) {
		case AREGTYPE:
		case REGTYPE:
		case CONTTYPE:
			size = strtobase(h_size(buf), 0, LONG_MAX, 8);
			fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT, (mode_t)m);
			if (fd < 0) {
				warn("open %s", fname);
				return -1;
			}
			break;
		case LNKTYPE:
			if (link(h_linkname(buf), fname) < 0) {
				warn("link %s %s", buf, fname);
				return -1;
			}
			break;
		case SYMTYPE:
			if (symlink(h_linkname(buf), fname) < 0) {
				warn("symlink %s %s", buf, fname);
				return -1;
			}
			break;
		case DIRTYPE:
			if (mkdir(fname, (mode_t)m) < 0 &&
			    errno != EEXIST) {
				warn("mkdir %s", fname);
				return -1;
			}
			break;
		case CHRTYPE:
		case BLKTYPE:
		case FIFOTYPE:
			maj = strtobase(h_major(buf), 0, LONG_MAX, 8);
			min = strtobase(h_minor(buf), 0, LONG_MAX, 8);
			typ = TYPE(*h_type(buf)) | m;
			if (mknod(fname, typ, makedev(maj, min)) < 0) {
				warn("mknod %s", fname);
				return -1;
			}
			break;
		default:
			return -1;
		}

		gid = strtobase(h_gid(buf), 0, LONG_MAX, 8);
		uid = strtobase(h_uid(buf), 0, LONG_MAX, 8);

		if (CHOWN(*h_type(buf), fname, gid, uid) < 0) {
			warn("(l)chown %s", fname);
			return -1;
		}

		mtim  = strtobase(h_mtime(buf), 0, LONG_MAX, 8);

		if (fd != -1) {
			for (; size > 0; size -= sizeof(buf)) {
				if (read(tarfd, buf, sizeof(buf)) < 0) {
					warn("read %s", fname);
					close(fd);
					return -1;
				}
				r = MIN(size, sizeof(buf));
				if (write(fd, buf, r) != r) {
					warn("write %s", fname);
					close(fd);
					return -1;
				}
			}
			close(fd);
			fd = -1;
		}

		tms[0].tv_sec  = tms[1].tv_sec  = mtim;
		tms[0].tv_nsec = tms[1].tv_nsec = 0;
		if (utimensat(AT_FDCWD, fname, tms, AT_SYMLINK_NOFOLLOW) < 0) {
			warn("utimensat %s", fname);
			return -1;
		}
	}

	return 0;
}

int
uncomp(int ifd, int ofd)
{
	z_stream strm;
	ssize_t size, rf;
	int zerr, rval;
	unsigned char ibuf[BUFSIZ], obuf[BUFSIZ];

	rval          = 0;
	strm.zalloc   = NULL;
	strm.zfree    = NULL;
	strm.opaque   = NULL;
	strm.avail_in = 0;
	strm.next_in  = NULL;
	zerr          = 0;

	if (inflateInit(&strm) < 0) {
		errno = ENOMEM;
		warn("inflateInit");
		goto failure;
	}

	while (zerr != Z_STREAM_END) {
		if ((rf = read(ifd, ibuf, sizeof(ibuf))) < 0) {
			warn("read");
			goto failure;
		}
		strm.avail_in = (unsigned)rf;
		strm.next_in  = ibuf;
		do {
			strm.avail_out = sizeof(obuf);
			strm.next_out  = obuf;

			switch (zerr = inflate(&strm, Z_NO_FLUSH)) {
			case Z_STREAM_ERROR:
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				warnx("invalid or incomplete deflate data");
				goto failure;
			}

			size = sizeof(obuf) - strm.avail_out;
			if (write(ofd, obuf, size) != size) {
				warn("write");
				goto failure;
			}
		} while (!strm.avail_out);
	}

	goto done;
failure:
	rval = -1;
done:
	inflateEnd(&strm);
	return rval;
}
