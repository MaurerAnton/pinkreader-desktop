#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>
#include "PostData.h"

class RedditClient {
public:
    void setToken(const std::string &tok) { token_ = tok; }
    std::vector<PostData> fetchPosts(const std::string &sub, const std::string &sort, int limit = 25);
    std::vector<std::string> searchSubreddits(const std::string &query, const std::string &sort, int limit = 10);
    std::vector<PostData> searchPosts(const std::string &query, const std::string &sort,
                                       const std::string &timeFilter = "", int limit = 25);

private:
    std::string httpGet(const std::string &url);
    std::string token_;
    static constexpr const char *ANON_BASE = "https://www.reddit.com";
};
