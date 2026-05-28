#include "PostListPanel.h"

PostListPanel::PostListPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    listView_ = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);
    listView_->AppendColumn("Title", wxLIST_FORMAT_LEFT, 250);
    listView_->AppendColumn("Score", wxLIST_FORMAT_RIGHT, 60);
    listView_->AppendColumn("Comments", wxLIST_FORMAT_RIGHT, 70);
    listView_->AppendColumn("Author", wxLIST_FORMAT_LEFT, 100);
    listView_->AppendColumn("Subreddit", wxLIST_FORMAT_LEFT, 100);
    sizer->Add(listView_, 1, wxEXPAND);
    SetSizer(sizer);
}

void PostListPanel::setPosts(const std::vector<PostData> &posts) {
    posts_ = posts;
    listView_->DeleteAllItems();
    for (size_t i = 0; i < posts_.size(); i++) {
        auto &p = posts_[i];
        listView_->InsertItem(i, wxString::FromUTF8(p.title));
        listView_->SetItem(i, 1, wxString::Format("%d", p.score));
        listView_->SetItem(i, 2, wxString::Format("%d", p.numComments));
        listView_->SetItem(i, 3, wxString::FromUTF8(p.author));
        listView_->SetItem(i, 4, wxString::FromUTF8(p.subreddit));
    }
}
