#include "MainFrame.h"
#include "PostListPanel.h"
#include "ImageViewPanel.h"
#include "SearchPanel.h"
#include "RedditClient.h"

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_REFRESH, MainFrame::onRefresh)
    EVT_MENU(wxID_ANY, MainFrame::onLogin)
wxEND_EVENT_TABLE()

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
    navMenu->Append(wxID_ANY, "Popular", "Browse r/popular");
    navMenu->Append(wxID_ANY, "All", "Browse r/all");
    auto *authMenu = new wxMenu();
    authMenu->Append(wxID_ANY, "&Login...", "OAuth login");
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(navMenu, "&Navigate");
    menuBar->Append(authMenu, "&Account");
    SetMenuBar(menuBar);
}

void MainFrame::setupLayout() {
    splitter_ = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    postList_ = new PostListPanel(splitter_);
    auto *rightPanel = new wxPanel(splitter_);
    auto *rightSizer = new wxBoxSizer(wxVERTICAL);
    searchPanel_ = new SearchPanel(rightPanel);
    imageView_ = new ImageViewPanel(rightPanel);
    rightSizer->Add(searchPanel_, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(imageView_, 1, wxEXPAND | wxALL, 5);
    rightPanel->SetSizer(rightSizer);
    splitter_->SplitVertically(postList_, rightPanel, 400);
}

void MainFrame::onSearch(wxCommandEvent &evt) {
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

void MainFrame::loadPosts(const std::string &subreddit, const std::string &sort) {
    currentSub_ = subreddit;
    SetTitle(wxString::Format("PinkReader Desktop — r/%s", subreddit));
    posts_ = client_->fetchPosts(subreddit, sort, 50);
    postList_->setPosts(posts_);
}
