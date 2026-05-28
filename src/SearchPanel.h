#pragma once
#include <wx/wx.h>
#include <wx/combobox.h>
#include <wx/spinctrl.h>
#include <string>

struct SearchParams {
    std::string query;
    std::string type = "subs";   // subs or posts
    std::string sort = "relevance";
    std::string timeFilter;
    int limit = 25;
    bool nsfw = false;
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
    wxCheckBox *nsfwCheck_ = nullptr;
    wxButton *searchBtn_ = nullptr;
};
