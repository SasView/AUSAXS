#include <utility/Curl.h>
#include <utility/Console.h>
#include <io/File.h>

void curl::download(const std::string& url, const io::File& path) {
    CURL *curl;
    CURLcode res = CURLE_FAILED_INIT;
    FILE *fp;
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(path.path().c_str(), "wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        fclose(fp);
    }

    if (res == CURLE_OK) {
        console::print_success("\tSuccessfully downloaded " + url + " to " + path);
    } else {
        console::print_warning("\tFailed to download " + url);
        exit(1);
    }
}