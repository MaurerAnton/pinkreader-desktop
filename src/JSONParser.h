#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include "PostData.h"

enum JType { JNULL, JBOOL, JNUM, JSTR, JARR, JOBJ };

struct JVal {
    JType type = JNULL;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<JVal> arr;
    std::map<std::string, JVal> obj;

    JVal operator[](const std::string &k) const {
        auto i = obj.find(k);
        return i != obj.end() ? i->second : JVal();
    }
    std::string str() const { return type == JSTR ? s : ""; }
    double num() const { return type == JNUM ? n : 0; }
    int i() const { return (int)n; }
    bool bl() const { return type == JBOOL ? b : false; }
    int sz() const { return type == JARR ? (int)arr.size() : type == JOBJ ? (int)obj.size() : 0; }
    JVal at(int i) const { return (type == JARR && i >= 0 && i < sz()) ? arr[i] : JVal(); }
};

struct JParser {
    const char *p, *e;
    JParser(const char *s, int l) : p(s), e(s + l) {}
    void ws() { while (p < e && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) p++; }
    JVal parse() {
        ws(); if (p >= e) return JVal();
        char c = *p;
        if (c == '"') return pstr();
        if (c == '{') return pobj();
        if (c == '[') return parr();
        if (c == 't' || c == 'f') return pbool();
        if (c == 'n') return pnull();
        return pnum();
    }
    JVal pnull() { if (e - p >= 4 && !memcmp(p, "null", 4)) { p += 4; return JVal(); } return JVal(); }
    JVal pbool() {
        JVal v; v.type = JBOOL;
        if (e - p >= 4 && !memcmp(p, "true", 4)) { v.b = true; p += 4; }
        else if (e - p >= 5 && !memcmp(p, "false", 5)) { v.b = false; p += 5; }
        return v;
    }
    JVal pnum() { JVal v; v.type = JNUM; char *ep; v.n = strtod(p, &ep); p = ep; return v; }
    JVal pstr() {
        JVal v; v.type = JSTR; p++;
        while (p < e && *p != '"') {
            if (*p == '\\') {
                p++; if (p < e) {
                    switch (*p) {
                        case 'n': v.s += '\n'; break; case 't': v.s += '\t'; break;
                        case 'r': v.s += '\r'; break; case '"': v.s += '"'; break;
                        case '\\': v.s += '\\'; break; case '/': v.s += '/'; break;
                        case 'u': {
                            if (e - p < 5) break;
                            char h[5] = {p[1],p[2],p[3],p[4],0};
                            unsigned cp = strtoul(h, nullptr, 16); p += 4;
                            if (cp < 0x80) v.s += (char)cp;
                            else if (cp < 0x800) { v.s += (char)(0xC0|cp>>6); v.s += (char)(0x80|(cp&0x3F)); }
                            else if (cp <= 0xFFFF) { v.s += (char)(0xE0|cp>>12); v.s += (char)(0x80|(cp>>6&0x3F)); v.s += (char)(0x80|(cp&0x3F)); }
                        }
                    }
                }
            } else { v.s += *p; }
            p++;
        }
        if (p < e) p++;
        return v;
    }
    JVal parr() {
        JVal v; v.type = JARR; p++; ws();
        if (p < e && *p == ']') { p++; return v; }
        while (true) { v.arr.push_back(parse()); ws(); if (p < e && *p == ',') { p++; continue; } if (p < e && *p == ']') { p++; break; } break; }
        return v;
    }
    JVal pobj() {
        JVal v; v.type = JOBJ; p++; ws();
        if (p < e && *p == '}') { p++; return v; }
        while (true) { JVal k = pstr(); ws(); if (p < e && *p == ':') p++; ws(); v.obj[k.s] = parse(); ws(); if (p < e && *p == ',') { p++; ws(); continue; } if (p < e && *p == '}') { p++; break; } break; }
        return v;
    }
};

inline JVal parseJSON(const std::string &s) {
    if (s.empty()) return JVal();
    return JParser(s.c_str(), (int)s.size()).parse();
}

inline PostData parsePost(const JVal &d) {
    PostData p;
    p.id = d["id"].str();
    p.title = d["title"].str();
    p.author = d["author"].str();
    p.subreddit = d["subreddit"].str();
    p.url = d["url"].str();
    p.permalink = d["permalink"].str();
    p.domain = d["domain"].str();
    p.selftext = d["selftext"].str();
    p.thumbnail = d["thumbnail"].str();
    p.postHint = d["post_hint"].str();
    p.score = d["score"].i();
    p.numComments = d["num_comments"].i();
    p.over18 = d["over_18"].bl();
    p.isSelf = d["is_self"].bl();
    p.created = d["created_utc"].num();
    return p;
}

inline std::vector<PostData> parseListing(const std::string &json) {
    std::vector<PostData> posts;
    JVal root = parseJSON(json);
    JVal data = root["data"];
    if (data.type != JOBJ) return posts;
    JVal ch = data["children"];
    if (ch.type != JARR) return posts;
    for (int i = 0; i < ch.sz(); i++) {
        JVal child = ch.at(i);
        if (child.type != JOBJ) continue;
        if (child["kind"].str() == "t3")
            posts.push_back(parsePost(child["data"]));
    }
    return posts;
}
