#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define EOL '\n'

ssize_t
fgetline(char *buf, size_t bsize, FILE *stream)
{
	int c;
	ssize_t n;

	for (n = 0; n < bsize; n++) {
		c = fgetc(stream);

		if (c == EOL)
			break;
		if (c == EOF)
			return (n ? n : EOF);

		buf[n] = c;
	}

	return n;
}

size_t
filetohash(FILE *fp)
{
	size_t hash = 5381;
	int ch;

	while ((ch = getc(fp)) != EOF)
		hash = ((hash << 5) + hash) ^ ch;

	return hash;
}

size_t
strtohash(char *str)
{
	size_t hash = 5381;
	int ch;

	while ((ch = *str++))
		hash = ((hash << 5) + hash) ^ ch;

	return hash;
}

long long
stoll(const char *str, long long min, long long max, int base)
{
	char *end;
	long long ll;

	errno = 0;
	ll = strtoll(str, &end, base);

	if (end == str || *end != '\0')
		errno = EINVAL;

	if (ll > max || ll < min)
		errno = ERANGE;

	if (errno)
		return -1;

	return ll;
}
