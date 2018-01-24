#include <curl/curl.h>

#include <stdio.h>

int curl_errno;

int
download(char *URL, FILE *fp, const char *flags)
{
	CURL *curl;
	int rval = 0;

	if (!(curl = curl_easy_init()))
		goto err;

	curl_easy_setopt(curl, CURLOPT_URL, URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	if ((curl_errno = curl_easy_perform(curl)))
		goto err;

	goto done;
err:
	rval = -1;
done:
	curl_easy_cleanup(curl);
	return rval;
}
