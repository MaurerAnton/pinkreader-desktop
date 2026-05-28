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
        wxString title = wxString::FromUTF8(p.title.c_str());
        fprintf(stderr, "[setPosts] %zu: %s\n", i, p.title.c_str()); fflush(stderr);
        listView_->InsertItem(i, title);
        listView_->SetItem(i, 1, wxString::Format("%d", p.score));
        listView_->SetItem(i, 2, wxString::Format("%d", p.numComments));
        listView_->SetItem(i, 3, wxString::FromUTF8(p.author.c_str()));
        listView_->SetItem(i, 4, wxString::FromUTF8(p.subreddit.c_str()));
    }
    fprintf(stderr, "[setPosts] done\n"); fflush(stderr);
}
