#include "RedditClient.h"
#include "JSONParser.h"
#include <sstream>
#include <cstdio>

std::vector<std::string> RedditClient::searchSubreddits(const std::string &query, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/subreddits/search.json?q=" + query
                    + "&sort=" + sort + "&limit=" + std::to_string(limit) + "&raw_json=1";
    auto body = httpGet(url);
    std::vector<std::string> names;
    if (body.empty() || body.size() < 10 || body[0] == '<') return names;
    JVal root = parseJSON(body);
    JVal ch = root["data"]["children"];
    for (int i = 0; i < ch.sz(); i++) {
        JVal d = ch.at(i)["data"];
        if (d.type == JOBJ) names.push_back(d["display_name"].str());
    }
    return names;
}

std::vector<PostData> RedditClient::searchPosts(const std::string &query, const std::string &sort,
                                                  const std::string &timeFilter, int limit) {
    std::string url = std::string(ANON_BASE) + "/search.json?q=" + query
                    + "&type=link&sort=" + sort + "&limit=" + std::to_string(limit) + "&raw_json=1";
    if (!timeFilter.empty()) url += "&t=" + timeFilter;
    auto body = httpGet(url);
    if (body.empty() || body.size() < 10 || body[0] == '<') return {};
    return parseListing(body);
}

std::vector<PostData> RedditClient::fetchPosts(const std::string &sub, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/r/" + sub + "/" + sort
                    + ".json?limit=" + std::to_string(limit) + "&raw_json=1";
    fprintf(stderr, "[fetchPosts] URL: %s\n", url.c_str()); fflush(stderr);
    auto body = httpGet(url);
    fprintf(stderr, "[fetchPosts] %s -> %zu bytes\n", sub.c_str(), body.size()); fflush(stderr);
    if (body.empty() || body.size() < 10) { fprintf(stderr, "[fetchPosts] body too small\n"); fflush(stderr); return {}; }
    if (body[0] == '<') {
        fprintf(stderr, "[fetchPosts] HTML response: %.200s\n", body.c_str()); fflush(stderr);
        return {};
    }
    fprintf(stderr, "[fetchPosts] body start: %.80s\n", body.c_str()); fflush(stderr);
    auto posts = parseListing(body);
    fprintf(stderr, "[fetchPosts] parsed %zu posts\n", posts.size()); fflush(stderr);
    return posts;
}

std::string RedditClient::httpGet(const std::string &url) {
    fprintf(stderr, "[httpGet] %s\n", url.c_str()); fflush(stderr);
    std::string body;
    auto *curl = curl_easy_init();
    if (!curl) { fprintf(stderr, "[httpGet] curl_easy_init failed\n"); fflush(stderr); return ""; }
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
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    fprintf(stderr, "[httpGet] HTTP %ld, body %zu bytes\n", code, body.size()); fflush(stderr);
    curl_easy_cleanup(curl);
    return body;
}
