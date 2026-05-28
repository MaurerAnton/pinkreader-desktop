#include "PostListPanel.h"
#include <cstdio>

PostListPanel::PostListPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    fprintf(stderr, "[PostListPanel] constructor\n"); fflush(stderr);
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    listView_ = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    fprintf(stderr, "[PostListPanel] listView created\n"); fflush(stderr);
    listView_->AppendColumn("Title", wxLIST_FORMAT_LEFT, 250);
    listView_->AppendColumn("Score", wxLIST_FORMAT_RIGHT, 60);
    listView_->AppendColumn("Comments", wxLIST_FORMAT_RIGHT, 70);
    listView_->AppendColumn("Author", wxLIST_FORMAT_LEFT, 100);
    listView_->AppendColumn("Subreddit", wxLIST_FORMAT_LEFT, 100);
    fprintf(stderr, "[PostListPanel] columns added\n"); fflush(stderr);
    sizer->Add(listView_, 1, wxEXPAND);
    SetSizer(sizer);
    fprintf(stderr, "[PostListPanel] done\n"); fflush(stderr);
}

wxListView *PostListPanel::getListView() { return listView_; }

void PostListPanel::setPosts(const std::vector<PostData> &posts) {
    fprintf(stderr, "[setPosts] %zu posts\n", posts.size()); fflush(stderr);
    posts_ = posts;
    listView_->DeleteAllItems();
    fprintf(stderr, "[setPosts] deleted all\n"); fflush(stderr);
    for (size_t i = 0; i < posts_.size(); i++) {
        auto &p = posts_[i];
        // Sanitize: validate UTF-8, replace invalid sequences
        auto clean = [](const std::string &s) -> wxString {
            std::string out;
            out.reserve(s.size());
            for (size_t j = 0; j < s.size(); ) {
                unsigned char c = s[j];
                // ASCII
                if (c < 0x80) {
                    if (c >= 0x20 || c == '\n' || c == '\t' || c == '\r') out += c;
                    j++;
                }
                // 2-byte sequence
                else if ((c & 0xE0) == 0xC0 && j + 1 < s.size() && (s[j+1] & 0xC0) == 0x80) {
                    out += s[j]; out += s[j+1]; j += 2;
                }
                // 3-byte sequence
                else if ((c & 0xF0) == 0xE0 && j + 2 < s.size() && (s[j+1] & 0xC0) == 0x80 && (s[j+2] & 0xC0) == 0x80) {
                    out += s[j]; out += s[j+1]; out += s[j+2]; j += 3;
                }
                // 4-byte sequence
                else if ((c & 0xF8) == 0xF0 && j + 3 < s.size() && (s[j+1] & 0xC0) == 0x80 && (s[j+2] & 0xC0) == 0x80 && (s[j+3] & 0xC0) == 0x80) {
                    out += s[j]; out += s[j+1]; out += s[j+2]; out += s[j+3]; j += 4;
                }
                else { j++; } // skip invalid byte
            }
            return wxString::FromUTF8(out.c_str());
        };
        wxString title = clean(p.title);
        fprintf(stderr, "[setPosts] %zu: %s\n", i, p.title.c_str()); fflush(stderr);
        listView_->InsertItem(i, title);
        listView_->SetItem(i, 1, wxString::Format("%d", p.score));
        listView_->SetItem(i, 2, wxString::Format("%d", p.numComments));
        listView_->SetItem(i, 3, clean(p.author));
        listView_->SetItem(i, 4, clean(p.subreddit));
    }
    fprintf(stderr, "[setPosts] done\n"); fflush(stderr);
}
