#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pkg.h"

#define BLKSIZE 512

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

static int
mkdirp(const char *path, mode_t dir_mode, mode_t mode)
{
	char *p, c;

	c = 0;
	p = (char *)path;

	if (*path == '.' || *path == '/')
		return 0;

	do {
		p += strspn(p, "/");
		p += strcspn(p, "/");

		c = *p;
		*p = '\0';

		if (mkdir(path, c ? dir_mode : mode) < 0 && errno != EEXIST)
			return -1;
	} while ((*p = c) != '\0');

	return 0;
}

int
unarchive(int tarfd)
{
	struct header *head;
	struct timespec times[2];
	ssize_t rh, r;
	long gid, major, minor, mode, mtime, size, type, uid;
	int fd, n, rval;
	char blk[BLKSIZE], buf[257], fname[257];

	fd   = -1;
	head = (struct header *)blk;
	n    = 0;
	rval = 0;

	while ((rh = read(tarfd, blk, BLKSIZE)) > 0 && head->name[0]) {
		if (head->prefix[0])
			n = snprintf(fname, sizeof(fname), "%.*s/",
			             (int)sizeof(head->prefix), head->prefix);
		snprintf(fname + n, sizeof(fname) - n, "%.*s",
		         (int)sizeof(head->name), head->name);

		if ((mode  = stoll(head->mode,  0, LONG_MAX, 8)) < 0 ||
		    (mtime = stoll(head->mtime, 0, LONG_MAX, 8)) < 0)
				goto failure;

		snprintf(buf, sizeof(buf), "%s", fname);
		if (mkdirp(dirname(buf), ACCESSPERMS, ACCESSPERMS) < 0)
			goto failure;

		switch (head->type) {
		case AREGTYPE:
		case REGTYPE:
		case CONTTYPE:
			if ((fd = open(fname, O_WRONLY|O_TRUNC|O_CREAT,
			    DEFFILEMODE)) < 0)
				goto failure;
			for (; size > 0; size -= BLKSIZE) {
				if ((r = read(fd, blk, MIN(size, BLKSIZE))) < 0)
					goto failure;
				if (write(fd, blk, r) != r)
					goto failure;
			}
			break;
		case LNKTYPE:
		case SYMTYPE:
			snprintf(buf, sizeof(buf), "%.*s",
			         (int)sizeof(head->linkname), head->linkname);
			if (((head->type == SYMTYPE) ?
			    link : symlink)(buf, fname) < 0)
				goto failure;
			break;
		case DIRTYPE:
			if (mkdir(fname, (mode_t)mode) < 0 && errno != EEXIST)
				goto failure;
			break;
		case CHRTYPE:
		case BLKTYPE:
		case FIFOTYPE:
			if ((major = stoll(head->major, 0, LONG_MAX, 8)) < 0 ||
			    (minor = stoll(head->minor, 0, LONG_MAX, 8)) < 0)
				goto failure;
			type = (head->type == CHRTYPE) ? S_IFCHR :
			       (head->type == BLKSIZE) ? S_IFBLK : S_IFIFO;
			if (mknod(fname, type|mode, makedev(major, minor)) < 0)
				goto failure;
			break;
		default:
			goto failure;
		}

		if ((gid = stoll(head->gid, 0, LONG_MAX, 8)) < 0 ||
		    (uid = stoll(head->uid, 0, LONG_MAX, 8)) < 0)
				goto failure;

		times[0].tv_sec  = times[1].tv_sec  = mtime;
		times[0].tv_nsec = times[1].tv_nsec = 0;
		if (utimensat(AT_FDCWD, fname, times, AT_SYMLINK_NOFOLLOW) < 0)
			goto failure;
		if ((head->type == SYMTYPE) && lchown(fname, uid, gid) < 0) {
			goto failure;
		} else {
			if (chown(fname, uid, gid) < 0 ||
			    chmod(fname, mode) < 0)
			goto failure;
		}

		if (fd != -1)
			close(fd);
	}

	if (rh < 0)
		goto failure;

	goto done;
failure:
	rval = -1;
done:
	if (fd != -1)
		close(fd);

	return rval;
}
