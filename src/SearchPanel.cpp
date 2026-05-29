#include "SearchPanel.h"

SearchPanel::SearchPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    auto *row1 = new wxBoxSizer(wxHORIZONTAL);

    input_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    input_->SetHint("Search subreddits or posts...");
    searchBtn_ = new wxButton(this, wxID_ANY, "Search");
    row1->Add(input_, 1, wxEXPAND | wxALL, 2);
    row1->Add(searchBtn_, 0, wxALL, 2);
    sizer->Add(row1, 0, wxEXPAND);

    auto *row2 = new wxBoxSizer(wxHORIZONTAL);

    typeBox_ = new wxComboBox(this, wxID_ANY, "Subreddits", wxDefaultPosition, wxDefaultSize,
                               0, nullptr, wxCB_READONLY);
    typeBox_->Append("Subreddits"); typeBox_->Append("Posts");
    typeBox_->SetSelection(0);
    row2->Add(new wxStaticText(this, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    row2->Add(typeBox_, 0, wxALL, 2);

    sortBox_ = new wxComboBox(this, wxID_ANY, "relevance", wxDefaultPosition, wxDefaultSize,
                               0, nullptr, wxCB_READONLY);
    sortBox_->Append("relevance"); sortBox_->Append("hot"); sortBox_->Append("new");
    sortBox_->Append("top"); sortBox_->Append("comments");
    sortBox_->SetSelection(0);
    row2->Add(new wxStaticText(this, wxID_ANY, "Sort:"), 0, wxALIGN_CENTER_VERTICAL, 0);
    row2->Add(sortBox_, 0, wxALL, 2);

    timeBox_ = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                               0, nullptr, wxCB_READONLY);
    timeBox_->Append("any time"); timeBox_->Append("hour"); timeBox_->Append("day");
    timeBox_->Append("week"); timeBox_->Append("month"); timeBox_->Append("year"); timeBox_->Append("all");
    timeBox_->SetSelection(0);
    row2->Add(new wxStaticText(this, wxID_ANY, "Time:"), 0, wxALIGN_CENTER_VERTICAL, 0);
    row2->Add(timeBox_, 0, wxALL, 2);

    limitSpin_ = new wxSpinCtrl(this, wxID_ANY, "25", wxDefaultPosition, wxSize(60, -1),
                                 wxSP_ARROW_KEYS, 1, 100, 25);
    row2->Add(new wxStaticText(this, wxID_ANY, "Limit:"), 0, wxALIGN_CENTER_VERTICAL, 0);
    row2->Add(limitSpin_, 0, wxALL, 2);

    sizer->Add(row2, 0, wxEXPAND | wxALL, 2);

    auto *row3 = new wxBoxSizer(wxHORIZONTAL);
    bestQualityCheck_ = new wxCheckBox(this, wxID_ANY, "Best quality");
    dedupCheck_ = new wxCheckBox(this, wxID_ANY, "Dedup");
    imagesOnlyCheck_ = new wxCheckBox(this, wxID_ANY, "Images only");
    minImgSpin_ = new wxSpinCtrl(this, wxID_ANY, "3", wxDefaultPosition, wxSize(50, -1),
                                  wxSP_ARROW_KEYS, 1, 50, 3);
    nsfwCheck_ = new wxCheckBox(this, wxID_ANY, "NSFW");
    torCheck_ = new wxCheckBox(this, wxID_ANY, "Tor");
    row3->Add(bestQualityCheck_, 0, wxALL, 2);
    row3->Add(dedupCheck_, 0, wxALL, 2);
    row3->Add(imagesOnlyCheck_, 0, wxALL, 2);
    row3->Add(minImgSpin_, 0, wxALL, 2);
    row3->Add(nsfwCheck_, 0, wxALL, 2);
    row3->Add(torCheck_, 0, wxALL, 2);
    sizer->Add(row3, 0, wxEXPAND | wxALL, 2);

    SetSizer(sizer);
}

SearchParams SearchPanel::getParams() const {
    SearchParams p;
    p.query = std::string(input_->GetValue().mb_str());
    p.type = (typeBox_->GetSelection() == 0) ? "subs" : "posts";
    p.sort = std::string(sortBox_->GetValue().mb_str());
    int ts = timeBox_->GetSelection();
    p.timeFilter = (ts <= 0) ? "" : std::string(timeBox_->GetString(ts).mb_str());
    p.limit = limitSpin_->GetValue();
    p.nsfw = nsfwCheck_->GetValue();
    p.bestQuality = bestQualityCheck_->GetValue();
    p.dedup = dedupCheck_->GetValue();
    p.imagesOnly = imagesOnlyCheck_->GetValue();
    p.minImages = minImgSpin_->GetValue();
    p.useTor = torCheck_->GetValue();
    return p;
}
