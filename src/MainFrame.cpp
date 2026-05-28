#include "MainFrame.h"
#include "PostListPanel.h"
#include "ImageViewPanel.h"
#include "SearchPanel.h"
#include "RedditClient.h"
#include <cstdio>

MainFrame::MainFrame(const wxString &title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1024, 700))
{
    fprintf(stderr, "[MainFrame] constructor start\n"); fflush(stderr);
    client_ = std::make_unique<RedditClient>();
    fprintf(stderr, "[MainFrame] RedditClient created\n"); fflush(stderr);
    setupMenu();
    fprintf(stderr, "[MainFrame] menu done\n"); fflush(stderr);
    setupLayout();
    fprintf(stderr, "[MainFrame] layout done\n"); fflush(stderr);
    CreateStatusBar();
    fprintf(stderr, "[MainFrame] statusbar done, loading posts\n"); fflush(stderr);
    loadPosts("popular", "hot");
    fprintf(stderr, "[MainFrame] posts loaded\n"); fflush(stderr);
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
}

void MainFrame::setupLayout() {
    splitter_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    postList_ = new PostListPanel(splitter_);
    postList_->getListView()->Bind(wxEVT_LIST_ITEM_SELECTED, &MainFrame::onPostSelected, this);

    auto *rightPanel = new wxPanel(splitter_);
    auto *rightSizer = new wxBoxSizer(wxVERTICAL);
    searchPanel_ = new SearchPanel(rightPanel);
    searchPanel_->getButton()->Bind(wxEVT_BUTTON, &MainFrame::onSearch, this);
    imageView_ = new ImageViewPanel(rightPanel);
    rightSizer->Add(searchPanel_, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(imageView_, 1, wxEXPAND | wxALL, 5);
    rightPanel->SetSizer(rightSizer);
    splitter_->SplitVertically(postList_, rightPanel, 400);
}

void MainFrame::onSearch(wxCommandEvent &) {
    std::string query = searchPanel_->getQuery();
    if (!query.empty()) loadPosts(query);
}

void MainFrame::onPostSelected(wxListEvent &evt) {
    int idx = evt.GetIndex();
    if (idx >= 0 && idx < (int)posts_.size()) {
        auto &p = posts_[idx];
        imageView_->showImage(p.url, p.title);
        wxLogStatus("r/%s — u/%s — %d pts, %d comments",
                     p.subreddit, p.author, p.score, p.numComments);
    }
}

void MainFrame::onRefresh(wxCommandEvent &) { loadPosts(currentSub_); }

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
    fprintf(stderr, "[loadPosts] %s/%s\n", subreddit.c_str(), sort.c_str()); fflush(stderr);
    currentSub_ = subreddit;
    fprintf(stderr, "[loadPosts] SetTitle...\n"); fflush(stderr);
    SetTitle(wxString::Format("PinkReader Desktop — r/%s", subreddit));
    fprintf(stderr, "[loadPosts] fetchPosts...\n"); fflush(stderr);
    posts_ = client_->fetchPosts(subreddit, sort, 50);
    fprintf(stderr, "[loadPosts] got %zu posts, calling setPosts\n", posts_.size()); fflush(stderr);
    postList_->setPosts(posts_);
    fprintf(stderr, "[loadPosts] done\n"); fflush(stderr);
}
