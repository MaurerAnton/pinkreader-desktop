#pragma once
#include <wx/wx.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <string>

struct SearchParams {
    std::string query;
    std::string type = "subs";
    std::string sort = "relevance";
    std::string timeFilter;
    int limit = 25;
    int minImages = 3;        // minimum image posts a subreddit must have
    bool nsfw = false;
    bool bestQuality = false;
    bool dedup = false;
    bool imagesOnly = false;
};

class SearchPanel : public wxPanel {
public:
    SearchPanel(wxWindow *parent);
    SearchParams getParams() const;
    wxButton *getSearchButton() { return searchBtn_; }

private:
    wxTextCtrl *input_ = nullptr;
    wxComboBox *typeBox_ = nullptr;
    wxComboBox *sortBox_ = nullptr;
    wxComboBox *timeBox_ = nullptr;
    wxSpinCtrl *limitSpin_ = nullptr;
    wxSpinCtrl *minImgSpin_ = nullptr;
    wxCheckBox *nsfwCheck_ = nullptr;
    wxCheckBox *bestQualityCheck_ = nullptr;
    wxCheckBox *dedupCheck_ = nullptr;
    wxCheckBox *imagesOnlyCheck_ = nullptr;
    wxButton *searchBtn_ = nullptr;
};
