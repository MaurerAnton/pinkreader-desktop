#include "ImageViewPanel.h"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

ImageViewPanel::ImageViewPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    caption_ = new wxStaticText(this, wxID_ANY, "");
    caption_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    status_ = new wxStaticText(this, wxID_ANY, "Select a post to view image");
    bitmap_ = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
    sizer->Add(caption_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(bitmap_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(status_, 0, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

void ImageViewPanel::showImage(const std::string &url, const std::string &caption) {
    caption_->SetLabel(wxString::FromUTF8(caption));
    if (url.find(".jpg") != std::string::npos ||
        url.find(".jpeg") != std::string::npos ||
        url.find(".png") != std::string::npos ||
        url.find(".gif") != std::string::npos ||
        url.find(".webp") != std::string::npos) {
        status_->SetLabel("Loading: " + wxString::FromUTF8(url));
        // TODO: download and display image via wxWebRequest or curl+wxImage
    } else {
        status_->SetLabel("URL: " + wxString::FromUTF8(url));
    }
}
