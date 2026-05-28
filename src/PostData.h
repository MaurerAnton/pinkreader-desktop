#pragma once
#include <string>

struct PostData {
    std::string id, title, author, subreddit, url, permalink, domain;
    std::string selftext, thumbnail, postHint;
    int score = 0, numComments = 0;
    bool over18 = false, isSelf = false;
    double created = 0;
};
