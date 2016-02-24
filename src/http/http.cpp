#include "http.h"
#include <curl/curl.h>

size_t writeurldata(char *ptr, size_t size, size_t nmemb, void* userdata) {
	size_t realsize = size * nmemb;
	std::string* userstring = (std::string*)userdata;
	std::string appendStr(ptr, realsize);
	userstring->append(appendStr);
	return realsize;
}

std::string GetHTTPContents(std::string url) {
	std::string respData;

	char errorBuffer[1000];

	CURL* curl = curl_easy_init();
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	printf("Getting URL: %s\n", url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeurldata);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respData);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (res != 0) {
		printf("Error: %s\n", errorBuffer);
	}

	return respData;
}