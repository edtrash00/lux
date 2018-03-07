#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdlib.h>

#include "pkg.h"

mode_t
strtomode(const char *str, mode_t mode)
{
	mode_t octal;
	char *end;

	octal = (mode_t)strtoul(str, &end, 8);

	if (!*end || octal > 07777)
		return 0755;

	mode  &= ~ALLPERMS;
	octal &=  ALLPERMS;

	return  ((octal | mode) & ~umask(0));
}
