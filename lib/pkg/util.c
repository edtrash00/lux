#include <errno.h>
#include <stdlib.h>

unsigned long
hash(char *str)
{
	size_t hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) ^ c;

	return hash;
}

long long
stoll(const char *str, long long min, long long max)
{
	char *end;
	long long ll;

	errno = 0;
	ll = strtoll(str, &end, 10);

	if (end == str || *end != '\0')
		errno = EINVAL;

	if (ll > max || ll < min)
		errno = ERANGE;

	if (errno)
		return -1;

	return ll;
}

