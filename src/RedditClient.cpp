#include "RedditClient.h"
#include "JSONParser.h"
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <cctype>

void RedditClient::trackRequest() {
    reqCount_++;
    time_t now = time(nullptr);
    if (now - minuteStart_ >= 60) { minuteStart_ = now; reqPerMin_ = 1; }
    else reqPerMin_++;
}

std::string RedditClient::bestQualityUrl(const std::string &url) {
    if (url.find("i.redd.it/") != std::string::npos ||
        url.find("preview.redd.it/") != std::string::npos) {
        size_t q = url.find('?');
        if (q != std::string::npos) return url.substr(0, q);
    }
    return url;
}

static std::string urlEncode(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += c;
        else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
            out += buf;
        }
    }
    return out;
}

std::vector<std::string> RedditClient::searchSubreddits(const std::string &query, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/subreddits/search.json?q=" + urlEncode(query)
                    + "&sort=" + sort + "&limit=" + std::to_string(limit) + "&raw_json=1";
    return parseSubredditNames(httpGet(url));
}

std::vector<std::string> RedditClient::parseSubredditNames(const std::string &body) {
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
    std::string url = std::string(ANON_BASE) + "/search.json?q=" + urlEncode(query)
                    + "&type=link&sort=" + sort + "&limit=" + std::to_string(limit) + "&raw_json=1";
    if (!timeFilter.empty()) url += "&t=" + timeFilter;
    return filterPosts(parseListing(httpGet(url)));
}

std::vector<PostData> RedditClient::fetchPosts(const std::string &sub, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/r/" + sub + "/" + sort
                    + ".json?limit=" + std::to_string(limit) + "&raw_json=1";
    auto body = httpGet(url);
    auto posts = filterPosts(::parseListing(body));

    // Fallback: try old.reddit.com if JSON API returned 403
    if (posts.empty() && !body.empty() && body[0] != '{') {
        std::string oldUrl = buildOldRedditUrl(sub, sort);
        fprintf(stderr, "[fallback] trying old.reddit.com: %s\n", oldUrl.c_str()); fflush(stderr);
        auto oldBody = httpGet(oldUrl);
        posts = filterPosts(parseOldRedditListing(oldBody));
    }
    return posts;
}

std::vector<PostData> RedditClient::filterPosts(std::vector<PostData> posts) {
    for (auto &p : posts) {
        if (bestQuality_) p.url = bestQualityUrl(p.url);
        if (dedup_ && seenPosts_.count(p.id)) { p.id.clear(); continue; }
        if (dedup_) seenPosts_.insert(p.id);
    }
    posts.erase(std::remove_if(posts.begin(), posts.end(),
                [](const PostData &p) { return p.id.empty(); }), posts.end());
    return posts;
}

std::string RedditClient::httpGet(const std::string &url) {
    fprintf(stderr, "[httpGet] %s\n", url.c_str()); fflush(stderr);
    std::string body;
    auto *curl = curl_easy_init();
    if (!curl) { fprintf(stderr, "[httpGet] curl init failed\n"); return ""; }
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
    fprintf(stderr, "[httpGet] HTTP %ld, %zu bytes\n", code, body.size()); fflush(stderr);
    curl_easy_cleanup(curl);
    trackRequest();
    return body;
}

// ── old.reddit.com HTML parser (fallback for 403) ─────────────────────────

static std::string buildOldRedditUrl(const std::string &sub, const std::string &sort) {
    if (sub == "popular") return "https://old.reddit.com/r/popular/" + sort + "/";
    if (sub == "all") return "https://old.reddit.com/r/all/" + sort + "/";
    return "https://old.reddit.com/r/" + sub + "/" + sort + "/";
}

static std::vector<PostData> parseOldRedditListing(const std::string &html) {
    std::vector<PostData> posts;
    size_t pos = 0;
    while (true) {
        pos = html.find("<div class=\"thing", pos);
        if (pos == std::string::npos) break;
        size_t end = html.find("</div>", pos + 200);
        if (end == std::string::npos) break;
        std::string chunk = html.substr(pos, end - pos + 6);
        pos = end;

        PostData p;
        auto attr = [&](const std::string &key) -> std::string {
            std::string k = "data-" + key + "=\"";
            size_t a = chunk.find(k);
            if (a == std::string::npos) return "";
            a += k.size();
            size_t b = chunk.find('"', a);
            return (b == std::string::npos) ? "" : chunk.substr(a, b - a);
        };
        std::string fn = attr("fullname");
        p.id = (fn.find("t3_") == 0) ? fn.substr(3) : fn;
        p.author = attr("author");
        p.subreddit = attr("subreddit");
        p.url = attr("url");
        p.permalink = attr("permalink");
        p.domain = attr("domain");
        p.score = std::atoi(attr("score").c_str());
        p.numComments = std::atoi(attr("comments-count").c_str());
        p.over18 = (attr("nsfw") == "true");

        size_t tp = chunk.find("<a class=\"title");
        if (tp != std::string::npos) {
            tp = chunk.find(">", tp);
            if (tp != std::string::npos) {
                tp++;
                size_t te = chunk.find("</a>", tp);
                if (te != std::string::npos) p.title = chunk.substr(tp, te - tp);
            }
        }
        if (!p.title.empty()) posts.push_back(p);
    }
    return posts;
}
