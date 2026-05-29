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

// ── old.reddit.com fallback helpers ────────────────────────────────────────

static std::string buildOldRedditUrl(const std::string &sub, const std::string &sort) {
    if (sub == "popular") return "https://old.reddit.com/r/popular/" + sort + "/";
    if (sub == "all") return "https://old.reddit.com/r/all/" + sort + "/";
    return "https://old.reddit.com/r/" + sub + "/" + sort + "/";
}

static std::vector<std::string> parseOldRedditSubreddits(const std::string &html) {
    fprintf(stderr, "[oldreddit-sub] html %zu bytes\n", html.size()); fflush(stderr);
    std::vector<std::string> names;
    size_t pos = 0;
    int found = 0;
    while (true) {
        size_t p1 = html.find("<div class=\" thing", pos);
        size_t p2 = html.find("<div class=\"thing", pos);
        if (p1 == std::string::npos && p2 == std::string::npos) break;
        pos = (p1 != std::string::npos) ? p1 : p2;
        found++;
        fprintf(stderr, "[oldreddit-sub] #%d found at offset %zu\n", found, pos); fflush(stderr);
        size_t tagEnd = html.find('>', pos);
        if (tagEnd == std::string::npos) break;
        // Find matching </div>
        int depth = 0;
        size_t end = tagEnd + 1;
        while (end < html.size()) {
            size_t no = html.find("<div", end);
            size_t nc = html.find("</div>", end);
            if (nc == std::string::npos) break;
            if (no != std::string::npos && no < nc) { end = no + 4; depth++; }
            else { if (depth == 0) { end = nc + 6; break; } end = nc + 6; depth--; }
        }
        std::string chunk = html.substr(pos, end - pos);
        pos = end;
        fprintf(stderr, "[oldreddit-sub] chunk %zu bytes\n", chunk.size()); fflush(stderr);
        // Extract subreddit name from <a class="title" >name</a>
        size_t tp = chunk.find("class=\"title\"");
        if (tp == std::string::npos) tp = chunk.find("<a class=\"title");
        if (tp != std::string::npos) {
            tp = chunk.find(">", tp);
            if (tp != std::string::npos) {
                tp++;
                size_t te = chunk.find("</a>", tp);
                if (te != std::string::npos) {
                    std::string name = chunk.substr(tp, te - tp);
                    if (!name.empty()) {
                        // Strip "r/" prefix and ": description" suffix
                        if (name.find("r/") == 0) name = name.substr(2);
                        size_t colon = name.find(':');
                        if (colon != std::string::npos) name = name.substr(0, colon);
                        names.push_back(name);
                        fprintf(stderr, "[oldreddit-sub] #%d name=%s\n", found, name.c_str()); fflush(stderr);
                    }
                }
            }
        }
    }
    return names;
}

static std::vector<PostData> parseOldRedditSearchResults(const std::string &html) {
    // Search results use <div class="search-result search-result-link" with data- attrs
    std::vector<PostData> posts;
    size_t pos = 0;
    while (true) {
        pos = html.find("search-result-link", pos);
        if (pos == std::string::npos) break;
        pos = html.rfind("<div", pos);
        size_t tagEnd = html.find('>', pos);
        if (tagEnd == std::string::npos) break;
        // Find next search-result or </div> after this div
        size_t next = html.find("search-result", tagEnd);
        size_t endDiv = html.find("</div>", tagEnd);
        size_t end = std::min(next != std::string::npos ? next : std::string::npos,
                              endDiv != std::string::npos ? endDiv : std::string::npos);
        if (end == std::string::npos) break;
        std::string chunk = html.substr(pos, end - pos);
        pos = end;
        fprintf(stderr, "[searchresult] chunk %zu bytes\n", chunk.size()); fflush(stderr);

        PostData p;
        auto attr = [&](const std::string &key) -> std::string {
            std::string k = "data-" + key + "=\"";
            size_t a = chunk.find(k);
            if (a == std::string::npos) return "";
            a += k.size();
            size_t b = chunk.find('"', a);
            return (b == std::string::npos) ? "" : chunk.substr(a, b - a);
        };
        p.id = attr("fullname");
        if (p.id.find("t3_") == 0) p.id = p.id.substr(3);
        p.author = attr("author");
        p.subreddit = attr("subreddit");
        p.url = attr("url");
        p.permalink = attr("permalink");
        p.domain = attr("domain");
        p.score = std::atoi(attr("score").c_str());
        p.numComments = std::atoi(attr("comments-count").c_str());
        p.over18 = (attr("nsfw") == "true");

        // Extract title from <a class="search-title">
        size_t tp = chunk.find("class=\"search-title\"");
        if (tp != std::string::npos) {
            tp = chunk.find(">", tp);
            if (tp != std::string::npos) {
                tp++;
                size_t te = chunk.find("</a>", tp);
                if (te != std::string::npos) p.title = chunk.substr(tp, te - tp);
            }
        }
        if (!p.title.empty()) {
            posts.push_back(p);
            fprintf(stderr, "[searchresult] %s\n", p.title.c_str()); fflush(stderr);
        }
    }
    return posts;
}

static std::vector<PostData> parseOldRedditListing(const std::string &html) {
    std::vector<PostData> posts;
    size_t pos = 0;
    int found = 0;
    while (true) {
        pos = html.find("<div class=\" thing", pos);
        if (pos == std::string::npos) {
            // Try alternative patterns
            pos = html.find("<div class=\"thing", pos);
            if (pos == std::string::npos) break;
        }
        found++;
        // Skip past the opening > of the thing div
        size_t tagEnd = html.find('>', pos);
        if (tagEnd == std::string::npos) break;
        // Find end: count nested <div> and </div>, starting from AFTER thing's opening tag
        int depth = 0;
        size_t end = tagEnd + 1;
        while (end < html.size()) {
            size_t nextOpen = html.find("<div", end);
            size_t nextClose = html.find("</div>", end);
            if (nextClose == std::string::npos) break;
            if (nextOpen != std::string::npos && nextOpen < nextClose) {
                end = nextOpen + 4; depth++;
            } else {
                if (depth == 0) { end = nextClose + 6; break; }
                end = nextClose + 6; depth--;
            }
        }
        std::string chunk = html.substr(pos, end - pos);
        pos = end;
        fprintf(stderr, "[oldreddit] thing #%d: %zu bytes, depth=%d\n", found, chunk.size(), depth); fflush(stderr);

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

        fprintf(stderr, "[oldreddit] id=%s author=%s sub=%s url=%.50s\n",
                p.id.c_str(), p.author.c_str(), p.subreddit.c_str(), p.url.c_str()); fflush(stderr);

        size_t tp = chunk.find("<a class=\"title");
        if (tp != std::string::npos) {
            tp = chunk.find(">", tp);
            if (tp != std::string::npos) {
                tp++;
                size_t te = chunk.find("</a>", tp);
                if (te != std::string::npos) p.title = chunk.substr(tp, te - tp);
            }
        }
        fprintf(stderr, "[oldreddit] title=%.60s\n", p.title.c_str()); fflush(stderr);
        if (!p.title.empty()) posts.push_back(p);
    }
    fprintf(stderr, "[oldreddit] found %d things, %zu posts with titles\n", found, posts.size()); fflush(stderr);
    return posts;
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
    auto names = parseSubredditNames(httpGet(url));
    if (names.empty() && (lastHttpCode_ == 403 || lastHttpCode_ == 429)) {
        std::string oldUrl = "https://old.reddit.com/subreddits/search?q=" + urlEncode(query);
        fprintf(stderr, "[fallback] sub search 403, trying old.reddit.com\n"); fflush(stderr);
        names = parseOldRedditSubreddits(httpGet(oldUrl));
        if (!names.empty()) fallbackUsed_ = true;
    }
    return names;
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
    auto body = httpGet(url);
    auto posts = filterPosts(::parseListing(body));
    if (posts.empty() && (lastHttpCode_ == 403 || lastHttpCode_ == 429)) {
        std::string oldUrl = "https://old.reddit.com/search?q=" + urlEncode(query)
                           + "&sort=" + sort + "&restrict_sr=off";
        fprintf(stderr, "[fallback] search %ld, trying old.reddit.com\n", lastHttpCode_); fflush(stderr);
        auto oldBody = httpGet(oldUrl);
        posts = filterPosts(parseOldRedditSearchResults(oldBody));
        if (!posts.empty()) fallbackUsed_ = true;
    }
    return posts;
}

std::vector<PostData> RedditClient::fetchPosts(const std::string &sub, const std::string &sort, int limit) {
    std::string url = std::string(ANON_BASE) + "/r/" + sub + "/" + sort
                    + ".json?limit=" + std::to_string(limit) + "&raw_json=1";
    auto body = httpGet(url);
    auto posts = filterPosts(::parseListing(body));
    if (posts.empty() && (lastHttpCode_ == 403 || lastHttpCode_ == 429)) {
        std::string oldUrl = buildOldRedditUrl(sub, sort);
        fprintf(stderr, "[fallback] HTTP 403, trying old.reddit.com\n"); fflush(stderr);
        auto oldBody = httpGet(oldUrl);
        posts = filterPosts(parseOldRedditListing(oldBody));
        if (!posts.empty()) fallbackUsed_ = true;
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

std::string RedditClient::resolveVideoUrl(const std::string &url) {
    // Try yt-dlp to extract direct video URL
    std::string cmd = "yt-dlp -g --no-playlist " + url + " 2>/dev/null";
    FILE *fp = popen(cmd.c_str(), "r");
    if (!fp) return "";
    char buf[4096];
    std::string result;
    while (fgets(buf, sizeof(buf), fp)) result += buf;
    pclose(fp);
    // Return first line (direct URL), strip whitespace
    size_t nl = result.find('\n');
    if (nl != std::string::npos) result = result.substr(0, nl);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
    return (result.find("http") == 0) ? result : "";
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
    lastHttpCode_ = code;
    fprintf(stderr, "[httpGet] HTTP %ld, %zu bytes\n", code, body.size()); fflush(stderr);
    curl_easy_cleanup(curl);
    trackRequest();
    return body;
}
