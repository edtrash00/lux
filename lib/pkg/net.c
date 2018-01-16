#include <curl/curl.h>

#include <stdio.h>

int curl_errno;

int
download(char *URL, const char *path, const char *flags)
{
	CURL *curl;
	FILE *file = NULL;
	int rval = 0;

	if (!(curl = curl_easy_init()))
		goto err;
	if (!(file = fopen(path, "w")))
		goto err;

	curl_easy_setopt(curl, CURLOPT_URL, URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	if ((curl_errno = curl_easy_perform(curl)))
		goto err;

	goto done;
err:
	rval       = -1;
done:
	if (file)
		fclose(file);
	curl_easy_cleanup(curl);
	return rval;
}
