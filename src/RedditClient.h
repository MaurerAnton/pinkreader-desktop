#pragma once
#include <string>
#include <vector>
#include <set>
#include <curl/curl.h>
#include "PostData.h"

class RedditClient {
public:
    void setToken(const std::string &tok) { token_ = tok; }
    void setBestQuality(bool v) { bestQuality_ = v; }
    void setDedup(bool v) { dedup_ = v; }
    std::vector<PostData> fetchPosts(const std::string &sub, const std::string &sort, int limit = 25);
    std::vector<std::string> searchSubreddits(const std::string &query, const std::string &sort, int limit = 10);
    std::vector<PostData> searchPosts(const std::string &query, const std::string &sort,
                                       const std::string &timeFilter = "", int limit = 25);
    int requestCount() const { return reqCount_; }
    int requestsPerMinute() const { return reqPerMin_; }
    static constexpr int RATE_LIMIT = 600;

private:
    std::string httpGet(const std::string &url);
    std::string bestQualityUrl(const std::string &url);
    std::vector<std::string> parseSubredditNames(const std::string &body);
    std::vector<PostData> filterPosts(std::vector<PostData> posts);
    std::string token_;
    bool bestQuality_ = false;
    bool dedup_ = false;
    std::set<std::string> seenPosts_;
    std::set<std::string> seenImages_;
    int reqCount_ = 0;
    int reqPerMin_ = 0;
    time_t minuteStart_ = 0;
    void trackRequest();
    static constexpr const char *ANON_BASE = "https://www.reddit.com";
};
