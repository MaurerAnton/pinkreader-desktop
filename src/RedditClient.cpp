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
        // Find next search-result-link as boundary (or end of listing)
        size_t next = html.find("search-result-link", tagEnd);
        size_t listingEnd = html.find("search-result-listing", tagEnd);
        if (listingEnd != std::string::npos) listingEnd = html.rfind("</div>", listingEnd);
        size_t end = next;
        if (end == std::string::npos || (listingEnd != std::string::npos && listingEnd < end))
            end = listingEnd;
        if (end == std::string::npos) end = html.size();
        std::string chunk = html.substr(pos, end - pos);
        pos = end;
        fprintf(stderr, "[searchresult] chunk %zu bytes\n", chunk.size()); fflush(stderr);
        // Debug: print first 300 chars of chunk
        fprintf(stderr, "[searchresult] %.300s\n", chunk.c_str()); fflush(stderr);

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
        // Extract from permalink href: /r/subreddit/comments/id/title
        size_t ph = chunk.find("/r/");
        if (ph != std::string::npos) {
            size_t ps = ph + 3;
            size_t pe = chunk.find("/", ps);
            if (pe != std::string::npos) p.subreddit = chunk.substr(ps, pe - ps);
            // Permalink
            size_t pc = chunk.find("\"", ph);
            if (pc != std::string::npos) {
                p.permalink = chunk.substr(ph, pc - ph);
                // Full URL from search-title href
                size_t ur = chunk.rfind("href=\"", ph);
                if (ur != std::string::npos) {
                    ur += 6;
                    size_t ue = chunk.find("\"", ur);
                    if (ue != std::string::npos) {
                        p.url = chunk.substr(ur, ue - ur);
                        if (p.url.find("old.reddit.com") != std::string::npos) {
                            // It's a text post — use the permalink as the main URL
                            p.url = "https://www.reddit.com" + p.permalink;
                            p.isSelf = true;
                        }
                    }
                }
            }
        }
        // Extract author from <a href="/user/..."> or data-author
        size_t au = chunk.find("/user/");
        if (au == std::string::npos) au = chunk.rfind("data-author=", chunk.find("search-title"));
        if (au != std::string::npos && chunk.rfind("data-author=", au) != std::string::npos) {
            au = chunk.rfind("data-author=\"", au);
            if (au == std::string::npos) {
                // Alternative: just parse /user/ path
                au = chunk.find("/user/");
                if (au != std::string::npos) {
                    au += 7;
                    size_t aue = chunk.find("\"", au);
                    if (aue == std::string::npos) aue = chunk.find("/", au);
                    if (aue != std::string::npos) p.author = chunk.substr(au, aue - au);
                }
            } else {
                au += 14;
                size_t aue = chunk.find("\"", au);
                if (aue != std::string::npos) p.author = chunk.substr(au, aue - au);
            }
        }
        // Extract score from data-score
        p.score = std::atoi(attr("score").c_str());
        p.numComments = std::atoi(attr("comments-count").c_str());
        p.over18 = (attr("nsfw") == "true");

        // Extract title from <a class="search-title">
        size_t tp = chunk.find("search-title");
        fprintf(stderr, "[searchresult] search-title at=%zu\n", tp); fflush(stderr);
        if (tp != std::string::npos) {
            tp = chunk.find(">", tp);
            if (tp != std::string::npos) {
                tp++;
                size_t te = chunk.find("</a>", tp);
                if (te != std::string::npos) p.title = chunk.substr(tp, te - tp);
            }
        }
        fprintf(stderr, "[searchresult] title=%.60s\n", p.title.c_str()); fflush(stderr);
        fprintf(stderr, "[searchresult] id=%s author=%s sub=%s score=%d\n",
                p.id.c_str(), p.author.c_str(), p.subreddit.c_str(), p.score); fflush(stderr);
        // Determine post hint from thumbnail/img in HTML
        if (p.postHint.empty()) {
            if (chunk.find(".jpg") != std::string::npos ||
                chunk.find(".jpeg") != std::string::npos ||
                chunk.find(".png") != std::string::npos ||
                chunk.find(".gif") != std::string::npos ||
                chunk.find(".webp") != std::string::npos)
                p.postHint = "image";
            else if (chunk.find("v.redd.it") != std::string::npos)
                p.postHint = "hosted:video";
            else if (p.isSelf || chunk.find("thumbnail self") != std::string::npos)
                p.postHint = "self";
            else
                p.postHint = "link";
        }
        // For image posts, extract the actual image URL from thumbnail
        if (p.postHint == "image") {
            size_t is = chunk.find("<img src=\"");
            if (is != std::string::npos) {
                is += 10;
                size_t ie = chunk.find("\"", is);
                if (ie != std::string::npos) {
                    std::string imgUrl = chunk.substr(is, ie - is);
                    // Decode HTML entities
                    size_t amp;
                    while ((amp = imgUrl.find("&amp;")) != std::string::npos)
                        imgUrl.replace(amp, 5, "&");
                    while ((amp = imgUrl.find("&#x27;")) != std::string::npos)
                        imgUrl.replace(amp, 6, "'");
                    while ((amp = imgUrl.find("&lt;")) != std::string::npos)
                        imgUrl.replace(amp, 4, "<");
                    while ((amp = imgUrl.find("&gt;")) != std::string::npos)
                        imgUrl.replace(amp, 4, ">");
                    if (imgUrl.find("http") != 0) imgUrl = "https:" + imgUrl;
                    // Strip query params for clean URL
                    size_t qm = imgUrl.find('?');
                    if (qm != std::string::npos) imgUrl = imgUrl.substr(0, qm);
                    p.url = imgUrl;
                }
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
        // Detect post type from URL/thumbnail for images-only filter
        if (p.url.find(".jpg") != std::string::npos ||
            p.url.find(".jpeg") != std::string::npos ||
            p.url.find(".png") != std::string::npos ||
            p.url.find(".gif") != std::string::npos ||
            p.url.find(".webp") != std::string::npos ||
            p.url.find("i.redd.it") != std::string::npos)
            p.postHint = "image";
        else if (p.url.find("v.redd.it") != std::string::npos)
            p.postHint = "hosted:video";
        else if (p.url.find("/r/") == 0 || p.url.find("/user/") == 0)
            p.postHint = "self";
        else if (p.url.find("http") == 0)
            p.postHint = "link";
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
    fprintf(stderr, "[filterPosts] in=%zu\n", posts.size()); fflush(stderr);
    for (auto &p : posts) {
        if (bestQuality_) p.url = bestQualityUrl(p.url);
        if (dedup_ && seenPosts_.count(p.id)) { p.id.clear(); continue; }
        if (dedup_) seenPosts_.insert(p.id);
    }
    posts.erase(std::remove_if(posts.begin(), posts.end(),
                [](const PostData &p) { return p.id.empty(); }), posts.end());
    fprintf(stderr, "[filterPosts] out=%zu\n", posts.size()); fflush(stderr);
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

void RedditClient::setProxy(const std::string &host, int port, bool socks) {
    proxyHost_ = host; proxyPort_ = port; proxySocks_ = socks;
}

void RedditClient::setTorProxy() {
    proxyHost_ = "127.0.0.1"; proxyPort_ = 9050; proxySocks_ = true;
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
    // Proxy support
    if (proxyPort_ > 0) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxyHost_.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYPORT, (long)proxyPort_);
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE,
                          proxySocks_ ? CURLPROXY_SOCKS5_HOSTNAME : CURLPROXY_HTTP);
        fprintf(stderr, "[httpGet] using proxy %s:%d (%s)\n",
                proxyHost_.c_str(), proxyPort_, proxySocks_ ? "socks5h" : "http"); fflush(stderr);
    }
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
