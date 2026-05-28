#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <memory>
#include <string>
#include <vector>

struct PostData {
    std::string id, title, author, subreddit, url, permalink, domain;
    std::string selftext, thumbnail, postHint;
    int score = 0, numComments = 0;
    bool over18 = false, isSelf = false;
    double created = 0;
};

class RedditClient;
class PostListPanel;
class ImageViewPanel;
class SearchPanel;

class MainFrame : public wxFrame {
public:
    MainFrame(const wxString &title);

private:
    void setupMenu();
    void setupLayout();
    void onSearch(wxCommandEvent &evt);
    void onPostSelected(wxListEvent &evt);
    void onRefresh();
    void onLogin(wxCommandEvent &evt);
    void loadPosts(const std::string &subreddit, const std::string &sort = "hot");

    wxSplitterWindow *splitter_ = nullptr;
    PostListPanel *postList_ = nullptr;
    ImageViewPanel *imageView_ = nullptr;
    SearchPanel *searchPanel_ = nullptr;
    std::unique_ptr<RedditClient> client_;
    std::vector<PostData> posts_;
    std::string currentSub_;
    std::string token_;

    wxDECLARE_EVENT_TABLE();
};
