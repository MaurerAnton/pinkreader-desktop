#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/gauge.h>
#include <wx/timer.h>
#include "PostData.h"
#include "SearchPanel.h"
#include <memory>
#include <string>
#include <vector>
#include <set>

enum {
    ID_MENU_POPULAR = wxID_HIGHEST + 1,
    ID_MENU_ALL,
    ID_MENU_LOGIN,
    ID_MENU_TOR,
    ID_CTX_OPEN_BROWSER,
    ID_CTX_OPEN_VIDEO,
    ID_CTX_COPY_LINK,
    ID_CTX_COPY_ID,
    ID_STATS_TIMER,
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
    void onTorProxy(wxCommandEvent &evt);
    void onNavPopular(wxCommandEvent &evt);
    void onNavAll(wxCommandEvent &evt);
    void onContextMenu(wxListEvent &evt);
    void onCtxOpenBrowser(wxCommandEvent &evt);
    void onCtxOpenVideo(wxCommandEvent &evt);
    void onCtxCopyLink(wxCommandEvent &evt);
    void onCtxCopyId(wxCommandEvent &evt);
    void onStatsTimer(wxTimerEvent &evt);
    void loadPosts(const std::string &subreddit, const std::string &sort = "hot");
    void doSearch(const SearchParams &params);

    bool isVideoPost(const PostData &p) const;
    void updateStats();
    void loadHistory();
    void saveHistory();
    void addToHistory(const std::string &entry);
    int lastContextIdx_ = -1;

    wxSplitterWindow *splitter_ = nullptr;
    PostListPanel *postList_ = nullptr;
    ImageViewPanel *imageView_ = nullptr;
    SearchPanel *searchPanel_ = nullptr;
    wxGauge *rateGauge_ = nullptr;
    wxStaticText *statsText_ = nullptr;
    wxTimer *statsTimer_ = nullptr;
    std::unique_ptr<RedditClient> client_;
    std::vector<PostData> posts_;
    std::vector<std::string> history_;
    std::string currentSub_;
    std::string token_;
    std::string historyFile_;
    std::string statsInfo_;
};
