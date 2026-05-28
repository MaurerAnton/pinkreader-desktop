#pragma once
#include <string>
#include <vector>
#include <curl/curl.h>
#include "MainFrame.h"

class RedditClient {
public:
    void setToken(const std::string &tok) { token_ = tok; }
    std::vector<PostData> fetchPosts(const std::string &sub, const std::string &sort, int limit = 25);

private:
    std::string httpGet(const std::string &url);
    PostData parsePost(const std::string &jsonChunk);
    std::string token_;
    static constexpr const char *ANON_BASE = "https://www.reddit.com";
};
