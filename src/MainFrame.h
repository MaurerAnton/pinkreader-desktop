#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <memory>
#include <string>
#include <vector>

enum {
    ID_MENU_POPULAR = wxID_HIGHEST + 1,
    ID_MENU_ALL,
    ID_MENU_LOGIN,
};

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
    void onRefresh(wxCommandEvent &evt);
    void onLogin(wxCommandEvent &evt);
    void onNavPopular(wxCommandEvent &evt);
    void onNavAll(wxCommandEvent &evt);
    void loadPosts(const std::string &subreddit, const std::string &sort = "hot");

    wxSplitterWindow *splitter_ = nullptr;
    PostListPanel *postList_ = nullptr;
    ImageViewPanel *imageView_ = nullptr;
    SearchPanel *searchPanel_ = nullptr;
    std::unique_ptr<RedditClient> client_;
    std::vector<PostData> posts_;
    std::string currentSub_;
    std::string token_;
};
