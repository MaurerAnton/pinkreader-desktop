#include "ImageViewPanel.h"
#include <curl/curl.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <cstdint>
#include <algorithm>

ImageViewPanel::ImageViewPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    caption_ = new wxStaticText(this, wxID_ANY, "");
    caption_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    status_ = new wxStaticText(this, wxID_ANY, "Select a post to view image");
    bitmap_ = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                  wxDefaultSize, wxBORDER_SIMPLE);
    sizer->Add(caption_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(bitmap_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(status_, 0, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

static size_t curlWrite(void *ptr, size_t sz, size_t nmemb, void *ud) {
    auto *v = (std::vector<uint8_t>*)ud;
    v->insert(v->end(), (uint8_t*)ptr, (uint8_t*)ptr + sz * nmemb);
    return sz * nmemb;
}

void ImageViewPanel::showImage(const std::string &url, const std::string &caption) {
    caption_->SetLabel(wxString(caption.c_str(), wxConvUTF8));
    bool isImage = (url.find(".jpg") != std::string::npos ||
                    url.find(".jpeg") != std::string::npos ||
                    url.find(".png") != std::string::npos ||
                    url.find(".gif") != std::string::npos ||
                    url.find(".webp") != std::string::npos ||
                    url.find("i.redd.it") != std::string::npos ||
                    url.find("preview.redd.it") != std::string::npos);
    if (!isImage) {
        status_->SetLabel("URL: " + wxString::FromUTF8(url));
        bitmap_->SetBitmap(wxNullBitmap);
        return;
    }

    status_->SetLabel("Downloading...");
    std::vector<uint8_t> data;
    auto *curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "PinkReader-Desktop/0.1");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    if (data.empty()) {
        status_->SetLabel("Failed to download");
        return;
    }

    wxMemoryInputStream mis(data.data(), data.size());
    wxImage img(mis, wxBITMAP_TYPE_ANY);
    if (!img.IsOk()) {
        status_->SetLabel("Failed to decode image (" +
                          wxString::Format("%zu", data.size()) + " bytes)");
        return;
    }

    int maxW = bitmap_->GetSize().GetWidth() - 10;
    int maxH = bitmap_->GetSize().GetHeight() - 10;
    if (maxW < 100) maxW = 400;
    if (maxH < 100) maxH = 300;
    if (img.GetWidth() > maxW || img.GetHeight() > maxH) {
        double s = std::min((double)maxW / img.GetWidth(), (double)maxH / img.GetHeight());
        img.Rescale((int)(img.GetWidth() * s), (int)(img.GetHeight() * s),
                     wxIMAGE_QUALITY_HIGH);
    }
    bitmap_->SetBitmap(wxBitmap(img));
    status_->SetLabel(wxString::Format("%dx%d  %.1f KB",
                        img.GetWidth(), img.GetHeight(), data.size() / 1024.0));
    Layout();
}
