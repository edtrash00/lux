#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"
#include "zlib.h"

#define BLKSIZE 512
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

struct header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char type;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char major[8];
	char minor[8];
	char prefix[155];
};

static void
sanitize(struct header *h)
{
	#define MEMSAN(a) \
	{\
		char *p;\
		if ((p = memchr((a), ' ', sizeof((a)))))\
			memset(p, 0, sizeof((a)) - (p - (a)));\
	}
	MEMSAN(h->mode);
	MEMSAN(h->gid);
	MEMSAN(h->size);
	MEMSAN(h->mtime);
	MEMSAN(h->chksum);
	MEMSAN(h->major);
	MEMSAN(h->minor);
}

int
unarchive(int tarfd)
{
	struct header *head;
	struct timespec tms[2];
	ssize_t r;
	long gid, major, minor, mode, mtime, size, type, uid;
	int n, fd;
	char blk[BLKSIZE], fname[256];

	fd   = -1;
	head = (struct header *)blk;

	while ((r = read(tarfd, blk, BLKSIZE)) > 0 && head->name[0]) {
		sanitize(head);

		n = 0;
		if (head->prefix[0])
			n = snprintf(fname, sizeof(fname), "%.*s/",
			             (int)sizeof(head->prefix), head->prefix);
		snprintf(fname + n, sizeof(fname) - n, "%.*s",
		         (int)sizeof(head->name), head->name);

		mode = strtomode(head->mode, ACCESSPERMS);

		if (mkdirp(dircomp(fname), ACCESSPERMS, ACCESSPERMS) < 0)
			return -1;

		switch (head->type) {
		case AREGTYPE:
		case REGTYPE:
		case CONTTYPE:
			size = strtobase(head->size, 0, LONG_MAX, 8);
			fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT, (mode_t)mode);
			if (fd < 0) {
				warn("open %s", fname);
				return -1;
			}
			break;
		case LNKTYPE:
			if (link(head->linkname, fname) < 0) {
				warn("link %s %s", head->linkname, fname);
				return -1;
			}
			break;
		case SYMTYPE:
			if (symlink(head->linkname, fname) < 0) {
				warn("symlink %s %s", head->linkname, fname);
				return -1;
			}
			break;
		case DIRTYPE:
			if (mkdir(fname, (mode_t)mode) < 0 && errno != EEXIST) {
				warn("mkdir %s", fname);
				return -1;
			}
			break;
		case CHRTYPE:
		case BLKTYPE:
		case FIFOTYPE:
			major = strtobase(head->major, 0, LONG_MAX, 8);
			minor = strtobase(head->minor, 0, LONG_MAX, 8);
			type = TYPE(head->type) | mode;
			if (mknod(fname, type, makedev(major, minor)) < 0) {
				warn("mknod %s", fname);
				return -1;
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}

		gid   = strtobase(head->gid, 0, LONG_MAX, 8);
		uid   = strtobase(head->uid, 0, LONG_MAX, 8);
		mtime = strtobase(head->mtime, 0, LONG_MAX, 8);

		if (fd != -1) {
			for (; size > 0; size -= sizeof(blk)) {
				if (read(tarfd, blk, sizeof(blk)) < 0) {
					warn("read %s", fname);
					close(fd);
					return -1;
				}
				r = MIN(size, sizeof(blk));
				if (write(fd, blk, r) != r) {
					warn("write %s", fname);
					close(fd);
					return -1;
				}
			}
			close(fd);
			fd = -1;
		}

		tms[0].tv_sec  = tms[1].tv_sec  = mtime;
		tms[0].tv_nsec = tms[1].tv_nsec = 0;
		if (utimensat(AT_FDCWD, fname, tms, AT_SYMLINK_NOFOLLOW) < 0) {
			warn("utimensat %s", fname);
			return -1;
		}
		if (CHOWN(head->type, fname, gid, uid) < 0) {
			warn("(l)chown %s", fname);
			return -1;
		}
	}

	if (r < 0) {
		warn("read");
		return -1;
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
