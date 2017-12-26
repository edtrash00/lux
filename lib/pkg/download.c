#include <curl/curl.h>

#include <stdio.h>

int curl_errno;

int
download(char *URL, const char *path, const char *flags)
{
	CURL *curl;
	CURLcode res = 0;
	FILE *file = NULL;
	int rval = 0;

	if (!(curl = curl_easy_init()))
		goto err;
	if (!(file = fopen(path, "w")))
		goto err;

	curl_easy_setopt(curl, CURLOPT_URL, URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
	if ((res = curl_easy_perform(curl)) > 0)
		goto err;

	goto done;
err:
	curl_errno = res;
	rval       = -1;
done:
	if (file)
		fclose(file);
	curl_easy_cleanup(curl);
	return rval;
}
