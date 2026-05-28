#include "RedditClient.h"
#include "JSONParser.h"
#include <sstream>
#include <cstdio>

std::vector<PostData> RedditClient::fetchPosts(const std::string &sub, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/r/" + sub + "/" + sort
                    + ".json?limit=" + std::to_string(limit) + "&raw_json=1";
    auto body = httpGet(url);
    fprintf(stderr, "[fetchPosts] %s -> %zu bytes\n", sub.c_str(), body.size());
    fflush(stderr);
    if (body.empty() || body.size() < 10) return {};
    auto posts = parseListing(body);
    fprintf(stderr, "[fetchPosts] parsed %zu posts\n", posts.size());
    fflush(stderr);
    return posts;
}

std::string RedditClient::httpGet(const std::string &url) {
    std::string body;
    auto *curl = curl_easy_init();
    if (!curl) return "";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](char *ptr, size_t sz, size_t nmemb, void *ud) -> size_t {
            ((std::string*)ud)->append(ptr, sz * nmemb);
            return sz * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PinkReader-Desktop/0.1");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (!token_.empty()) {
        struct curl_slist *h = nullptr;
        h = curl_slist_append(h, ("Authorization: Bearer " + token_).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h);
        curl_easy_perform(curl);
        curl_slist_free_all(h);
    } else {
        curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
    return body;
}
