#pragma once
#include <wx/wx.h>
#include <string>

class SearchPanel : public wxPanel {
public:
    SearchPanel(wxWindow *parent);
    std::string getQuery() const;

private:
    wxTextCtrl *input_ = nullptr;
    wxButton *searchBtn_ = nullptr;
};
