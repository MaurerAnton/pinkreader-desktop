#include "MainFrame.h"
#include "PostListPanel.h"
#include "ImageViewPanel.h"
#include "SearchPanel.h"
#include "RedditClient.h"
#include <cstdio>
#include <wx/clipbrd.h>
#include <wx/utils.h>

MainFrame::MainFrame(const wxString &title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 700))
{
    client_ = std::make_unique<RedditClient>();
    setupMenu();
    setupLayout();
    CreateStatusBar();
    loadPosts("popular", "hot");
}

void MainFrame::setupMenu() {
    auto *menuBar = new wxMenuBar();
    auto *fileMenu = new wxMenu();
    fileMenu->Append(wxID_REFRESH, "&Refresh\tF5");
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+X");
    auto *navMenu = new wxMenu();
    navMenu->Append(ID_MENU_POPULAR, "&Popular", "Browse r/popular");
    navMenu->Append(ID_MENU_ALL, "&All", "Browse r/all");
    auto *authMenu = new wxMenu();
    authMenu->Append(ID_MENU_LOGIN, "&Login...", "OAuth login");
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(navMenu, "&Navigate");
    menuBar->Append(authMenu, "&Account");
    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &MainFrame::onRefresh, this, wxID_REFRESH);
    Bind(wxEVT_MENU, [this](wxCommandEvent &) { Close(true); }, wxID_EXIT);
    Bind(wxEVT_MENU, &MainFrame::onNavPopular, this, ID_MENU_POPULAR);
    Bind(wxEVT_MENU, &MainFrame::onNavAll, this, ID_MENU_ALL);
    Bind(wxEVT_MENU, &MainFrame::onLogin, this, ID_MENU_LOGIN);
    Bind(wxEVT_MENU, &MainFrame::onCtxOpenBrowser, this, ID_CTX_OPEN_BROWSER);
    Bind(wxEVT_MENU, &MainFrame::onCtxOpenVideo, this, ID_CTX_OPEN_VIDEO);
    Bind(wxEVT_MENU, &MainFrame::onCtxCopyLink, this, ID_CTX_COPY_LINK);
    Bind(wxEVT_MENU, &MainFrame::onCtxCopyId, this, ID_CTX_COPY_ID);
}

void MainFrame::setupLayout() {
    splitter_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    postList_ = new PostListPanel(splitter_);
    postList_->getListView()->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::onPostSelected, this);
    postList_->getListView()->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &MainFrame::onContextMenu, this);

    auto *rightPanel = new wxPanel(splitter_);
    auto *rightSizer = new wxBoxSizer(wxVERTICAL);
    searchPanel_ = new SearchPanel(rightPanel);
    searchPanel_->getSearchButton()->Bind(wxEVT_BUTTON, &MainFrame::onSearch, this);
    imageView_ = new ImageViewPanel(rightPanel);
    rightSizer->Add(searchPanel_, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(imageView_, 1, wxEXPAND | wxALL, 5);
    rightPanel->SetSizer(rightSizer);
    splitter_->SplitVertically(postList_, rightPanel, 400);
}

void MainFrame::onSearch(wxCommandEvent &) {
    SearchParams params = searchPanel_->getParams();
    if (!params.query.empty()) doSearch(params);
}

void MainFrame::doSearch(const SearchParams &params) {
    client_->setBestQuality(params.bestQuality);
    client_->setDedup(params.dedup);
    if (params.type == "subs") {
        auto subs = client_->searchSubreddits(params.query, params.sort, params.limit);
        // Images-only: probe each subreddit for image content
        if (params.imagesOnly) {
            std::vector<std::string> imgSubs;
            for (auto &s : subs) {
                auto posts = client_->fetchPosts(s, "hot", 3);
                bool hasImage = false;
                for (auto &p : posts) {
                    if (p.postHint == "image" ||
                        p.url.find("i.redd.it") != std::string::npos ||
                        p.url.find(".jpg") != std::string::npos ||
                        p.url.find(".png") != std::string::npos) {
                        hasImage = true; break;
                    }
                }
                if (hasImage) imgSubs.push_back(s);
            }
            subs = imgSubs;
        }
        if (subs.empty()) {
            wxLogStatus("No subreddits found for: " + wxString::FromUTF8(params.query));
            return;
        }
        wxArrayString choices;
        for (auto &s : subs) choices.Add(wxString::FromUTF8(s));
        wxSingleChoiceDialog dlg(this, "Select a subreddit:", "Search Results", choices);
        if (dlg.ShowModal() == wxID_OK) {
            std::string sel = std::string(dlg.GetStringSelection().mb_str());
            loadPosts(sel, "hot");
        }
    } else {
        auto posts = client_->searchPosts(params.query, params.sort, params.timeFilter, params.limit);
        if (params.imagesOnly) {
            posts.erase(std::remove_if(posts.begin(), posts.end(),
                [](const PostData &p) { return p.postHint != "image"; }), posts.end());
        }
        if (posts.empty()) {
            wxLogStatus("No posts found for: " + wxString::FromUTF8(params.query));
            return;
        }
        currentSub_ = "search:" + params.query;
        SetTitle("PinkReader Desktop");
        posts_ = posts;
        postList_->setPosts(posts_);
        wxLogStatus(wxString::Format("r/%s - %zu posts | API: %d/%d%s",
                      params.query, posts_.size(), client_->requestCount(), RedditClient::RATE_LIMIT,
                      client_->fallbackUsed() ? " (via old.reddit.com)" : ""));
    }
}

void MainFrame::onPostSelected(wxListEvent &evt) {
    int idx = evt.GetIndex();
    if (idx >= 0 && idx < (int)posts_.size()) {
        auto &p = posts_[idx];
        if (isVideoPost(p))
            imageView_->showVideoInfo(p.url, p.title);
        else if (p.isGallery && !p.galleryUrls.empty())
            imageView_->showGallery(p.galleryUrls, p.title);
        else
            imageView_->showImage(p.url, p.title);
        wxLogStatus(wxString::Format("r/%s - u/%s - %d pts, %d comments",
                     p.subreddit.c_str(), p.author.c_str(), p.score, p.numComments));
    }
}

void MainFrame::onContextMenu(wxListEvent &evt) {
    lastContextIdx_ = evt.GetIndex();
    if (lastContextIdx_ < 0 || lastContextIdx_ >= (int)posts_.size()) return;
    auto &p = posts_[lastContextIdx_];

    wxMenu menu;
    menu.Append(ID_CTX_OPEN_BROWSER, "Open in browser");
    if (isVideoPost(p)) {
        menu.Append(ID_CTX_OPEN_VIDEO, "Play video (mpv)");
    }
    menu.AppendSeparator();
    menu.Append(ID_CTX_COPY_LINK, "Copy link");
    menu.Append(ID_CTX_COPY_ID, "Copy post ID");
    PopupMenu(&menu);
}

bool MainFrame::isVideoPost(const PostData &p) const {
    if (p.postHint == "hosted:video" || p.postHint == "rich:video") return true;
    if (p.domain == "v.redd.it") return true;
    std::string lower = p.url;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return (lower.find(".mp4") != std::string::npos ||
            lower.find(".webm") != std::string::npos ||
            lower.find("youtube.com") != std::string::npos ||
            lower.find("youtu.be") != std::string::npos);
}

void MainFrame::onCtxOpenBrowser(wxCommandEvent &) {
    if (lastContextIdx_ >= 0 && lastContextIdx_ < (int)posts_.size()) {
        std::string url = "https://www.reddit.com" + posts_[lastContextIdx_].permalink;
        wxLaunchDefaultBrowser(wxString::FromUTF8(url));
    }
}

void MainFrame::onCtxOpenVideo(wxCommandEvent &) {
    if (lastContextIdx_ >= 0 && lastContextIdx_ < (int)posts_.size()) {
        std::string url = posts_[lastContextIdx_].url;
        std::string directUrl = RedditClient::resolveVideoUrl(url);
        if (!directUrl.empty())
            wxExecute("mpv " + wxString::FromUTF8(directUrl));
        else
            wxLaunchDefaultBrowser(wxString::FromUTF8(url));
    }
}

void MainFrame::onCtxCopyLink(wxCommandEvent &) {
    if (lastContextIdx_ >= 0 && lastContextIdx_ < (int)posts_.size()) {
        std::string url = "https://www.reddit.com" + posts_[lastContextIdx_].permalink;
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(url)));
            wxTheClipboard->Close();
        }
    }
}

void MainFrame::onCtxCopyId(wxCommandEvent &) {
    if (lastContextIdx_ >= 0 && lastContextIdx_ < (int)posts_.size()) {
        std::string id = posts_[lastContextIdx_].id;
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(wxString::FromUTF8(id)));
            wxTheClipboard->Close();
        }
    }
}

void MainFrame::onRefresh(wxCommandEvent &) {
    auto params = searchPanel_->getParams();
    loadPosts(currentSub_, params.sort);
}

void MainFrame::onLogin(wxCommandEvent &) {
    wxTextEntryDialog dlg(this, "Enter OAuth Bearer token:", "PinkReader Login");
    if (dlg.ShowModal() == wxID_OK) {
        token_ = std::string(dlg.GetValue().mb_str());
        client_->setToken(token_);
    }
}

void MainFrame::onNavPopular(wxCommandEvent &) { loadPosts("popular"); }
void MainFrame::onNavAll(wxCommandEvent &) { loadPosts("all"); }

void MainFrame::loadPosts(const std::string &subreddit, const std::string &sort) {
    currentSub_ = subreddit;
    SetTitle("PinkReader Desktop");
    auto params = searchPanel_->getParams();
    client_->setBestQuality(params.bestQuality);
    client_->setDedup(params.dedup);
    posts_ = client_->fetchPosts(subreddit, sort.empty() ? params.sort : sort,
                                  params.limit > 0 ? params.limit : 50);
    if (params.imagesOnly) {
        posts_.erase(std::remove_if(posts_.begin(), posts_.end(),
            [](const PostData &p) {
                if (p.postHint == "image") return false; // keep
                // Also check URL for image extensions and domains
                if (p.url.find("i.redd.it") != std::string::npos) return false;
                if (p.url.find("i.imgur.com") != std::string::npos) return false;
                if (p.url.find(".jpg") != std::string::npos ||
                    p.url.find(".jpeg") != std::string::npos ||
                    p.url.find(".png") != std::string::npos ||
                    p.url.find(".gif") != std::string::npos ||
                    p.url.find(".webp") != std::string::npos) return false;
                // For old.reddit fallback: keep if domain is image host
                if (p.domain == "i.redd.it" || p.domain == "i.imgur.com") return false;
                return true; // remove
            }), posts_.end());
    }
    postList_->setPosts(posts_);
    wxLogStatus(wxString::Format("r/%s - %zu posts | API: %d/%d%s",
                  subreddit, posts_.size(), client_->requestCount(), RedditClient::RATE_LIMIT,
                  client_->fallbackUsed() ? " (via old.reddit.com)" : ""));
}
