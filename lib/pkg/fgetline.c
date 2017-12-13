#include <stdio.h>

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
