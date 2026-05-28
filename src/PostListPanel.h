#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "PostData.h"
#include <vector>

class PostListPanel : public wxPanel {
public:
    PostListPanel(wxWindow *parent);
    void setPosts(const std::vector<PostData> &posts);
    wxListView *getListView();

private:
    wxListView *listView_ = nullptr;
    std::vector<PostData> posts_;
};
