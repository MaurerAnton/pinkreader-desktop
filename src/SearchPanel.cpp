#include "SearchPanel.h"

SearchPanel::SearchPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxHORIZONTAL);
    input_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    searchBtn_ = new wxButton(this, wxID_ANY, "Search");
    sizer->Add(input_, 1, wxEXPAND | wxALL, 2);
    sizer->Add(searchBtn_, 0, wxALL, 2);
    SetSizer(sizer);
}

std::string SearchPanel::getQuery() const {
    return std::string(input_->GetValue().mb_str());
}
