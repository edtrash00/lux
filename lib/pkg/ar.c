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

#include "zlib.h"
#include "pkg.h"

#define BLKSIZE 512
#define LINK(a) ((a == SYMTYPE) ? symlink : link)

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

int
unarchive(int tarfd)
{
	struct header *head;
	struct timespec tms[2];
	ssize_t rh, r;
	long gid, major, minor, mode, mtime, size, type, uid;
	int fd, rval;
	char blk[BLKSIZE], buf[257], fname[257];

	fd   = -1;
	head = (struct header *)blk;
	rval = 0;

	while ((rh = read(tarfd, blk, BLKSIZE)) > 0 && head->name[0]) {
		fname[0] = '\0';

		if (head->prefix[0])
			strncat(fname, head->prefix, sizeof(head->prefix));

		strncat(fname, head->name, sizeof(head->name));

		mode  = strtomode(head->mode, ACCESSPERMS);

		snprintf(buf, sizeof(buf), "%s", fname);
		if (mkdirp(dirname(buf), ACCESSPERMS, ACCESSPERMS) < 0)
			goto failure;

		switch (head->type) {
		case AREGTYPE:
		case REGTYPE:
		case CONTTYPE:
			size = strtobase(head->size, 0, LONG_MAX, 8);
			fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT, DEFFILEMODE);
			if (fd < 0)
				goto failure;
			break;
		case LNKTYPE:
		case SYMTYPE:
			strncpy(buf, head->linkname, sizeof(head->linkname));
			if (LINK(head->type)(buf, fname) < 0) {
				warn("(sym)link %s %s", buf, fname);
				goto failure;
			}
			break;
		case DIRTYPE:
			if (mkdir(fname, (mode_t)mode) < 0 && errno != EEXIST) {
				warn("mkdir %s", fname);
				goto failure;
			}
			break;
		case CHRTYPE:
		case BLKTYPE:
		case FIFOTYPE:
			major = strtobase(head->major, 0, LONG_MAX, 8);
			minor = strtobase(head->minor, 0, LONG_MAX, 8);
			type  = (head->type == CHRTYPE) ? S_IFCHR :
			        (head->type == BLKTYPE) ? S_IFBLK : S_IFIFO;
			type |= mode;
			if (mknod(fname, type, makedev(major, minor)) < 0) {
				warn("mknod %s", fname);
				goto failure;
			}
			break;
		default:
			goto failure;
		}

		gid   = strtobase(head->gid, 0, LONG_MAX, 8);
		uid   = strtobase(head->uid, 0, LONG_MAX, 8);
		mtime = strtobase(head->mtime, 0, LONG_MAX, 8);

		if (fd != -1) {
			for (; size > 0; size -= sizeof(blk)) {
				if ((r = read(tarfd, blk, sizeof(blk))) < 0) {
					warn("read %s", fname);
					goto failure;
				}
				if (write(fd, blk, MIN(r, sizeof(blk))) != r) {
					warn("write %s", fname);
					goto failure;
				}
			}
			close(fd);
		}

		tms[0].tv_sec  = tms[1].tv_sec  = mtime;
		tms[0].tv_nsec = tms[1].tv_nsec = 0;
		if (utimensat(AT_FDCWD, fname, tms, AT_SYMLINK_NOFOLLOW) < 0) {
			warn("utimensat %s", fname);
			goto failure;
		}
		if (head->type == SYMTYPE) {
			if (!getuid() && lchown(fname, uid, gid)) {
				warn("lchown %s", fname);
				goto failure;
			}
		} else {
			if (!getuid() && chown(fname, uid, gid) < 0) {
				warn("chown %s", fname);
				goto failure;
			}
			if (chmod(fname, mode) < 0) {
				warn("chmod %s", fname);
				goto failure;
			}
		}
	}

	if (rh < 0) {
		warn("read");
		goto failure;
	}

	goto done;
failure:
	rval = -1;
done:
	return rval;
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
			if ((zerr = inflate(&strm, Z_NO_FLUSH)) < 0) {
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
