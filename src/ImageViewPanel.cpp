#include "ImageViewPanel.h"
#include "RedditClient.h"
#include <curl/curl.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/utils.h>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include <signal.h>

ImageViewPanel::ImageViewPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY)
{
    auto *sizer = new wxBoxSizer(wxVERTICAL);
    caption_ = new wxStaticText(this, wxID_ANY, "");
    caption_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    status_ = new wxStaticText(this, wxID_ANY, "Select a post to view");

    bitmap_ = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                  wxSize(400, 300), wxBORDER_SIMPLE);
    bitmap_->SetMinSize(wxSize(100, 100));

    videoPanel_ = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(400, 300));
    videoPanel_->SetBackgroundColour(*wxBLACK);
    videoPanel_->Hide();

    playBtn_ = new wxButton(this, wxID_ANY, "▶ Load Video");
    playBtn_->Hide();
    playBtn_->Bind(wxEVT_BUTTON, &ImageViewPanel::onPlayClick, this);

    sizer->Add(caption_, 0, wxEXPAND | wxALL, 5);
    sizer->Add(bitmap_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(videoPanel_, 1, wxEXPAND | wxALL, 5);
    sizer->Add(playBtn_, 0, wxALIGN_CENTER | wxALL, 5);
    sizer->Add(status_, 0, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

ImageViewPanel::~ImageViewPanel() { killMpv(); }

void ImageViewPanel::killMpv() {
    if (mpvPid_ > 0) { kill(mpvPid_, SIGTERM); mpvPid_ = 0; }
}

void ImageViewPanel::onPlayClick(wxCommandEvent &) {
    if (!pendingVideoUrl_.empty()) {
        killMpv();
        status_->SetLabel("Downloading video via yt-dlp...");
        std::string tmpFile = "/tmp/pinkreader_video_" + std::to_string(time(nullptr)) + ".mp4";
        std::string cmd = "yt-dlp -o " + tmpFile + " --no-playlist " + pendingVideoUrl_ + " 2>/dev/null";
        int rc = system(cmd.c_str());
        if (rc == 0 && wxFileExists(wxString::FromUTF8(tmpFile))) {
            wxYield();
            // Launch mpv with downloaded file (external window)
            cmd = "mpv --force-window=yes --loop=inf " + tmpFile + " &";
            mpvPid_ = wxExecute(wxString::FromUTF8(cmd));
            status_->SetLabel("Playing...");
            playBtn_->SetLabel("⏸ Pause");
        } else {
            status_->SetLabel("Failed to download video");
        }
    }
}

void ImageViewPanel::showVideoInfo(const std::string &url, const std::string &caption) {
    killMpv();
    lastVideoUrl_ = url;
    std::string cleanCaption;
    for (char c : caption) if ((unsigned char)c >= 0x20 && (unsigned char)c < 0x7F) cleanCaption += c;
    caption_->SetLabel(wxString::FromAscii(("[VIDEO] " + cleanCaption).c_str()));

    bitmap_->Hide();
    videoPanel_->Show();
    playBtn_->Show();
    playBtn_->SetLabel("▶ Load Video");

    pendingVideoUrl_ = RedditClient::resolveVideoUrl(url);
    if (pendingVideoUrl_.empty()) pendingVideoUrl_ = url;

    status_->SetLabel("Video: click Load Video to play");
    Layout();
}

static size_t curlWrite(void *ptr, size_t sz, size_t nmemb, void *ud) {
    auto *v = (std::vector<uint8_t>*)ud;
    v->insert(v->end(), (uint8_t*)ptr, (uint8_t*)ptr + sz * nmemb);
    return sz * nmemb;
}

void ImageViewPanel::showImage(const std::string &url, const std::string &caption) {
    killMpv();
    lastVideoUrl_.clear();
    pendingVideoUrl_.clear();
    playBtn_->Hide();
    videoPanel_->Hide();
    bitmap_->Show();
    std::string cleanCaption;
    for (char c : caption) if ((unsigned char)c >= 0x20 && (unsigned char)c < 0x7F) cleanCaption += c;
    caption_->SetLabel(wxString::FromAscii(cleanCaption.c_str()));

    bool isImage = (url.find(".jpg") != std::string::npos ||
                    url.find(".jpeg") != std::string::npos ||
                    url.find(".png") != std::string::npos ||
                    url.find(".gif") != std::string::npos ||
                    url.find(".webp") != std::string::npos ||
                    url.find(".bmp") != std::string::npos ||
                    url.find("i.redd.it") != std::string::npos ||
                    url.find("preview.redd.it") != std::string::npos ||
                    url.find("i.imgur.com") != std::string::npos);
    if (!isImage) {
        status_->SetLabel("Not an image: " + wxString::FromAscii(url.c_str()));
        bitmap_->SetBitmap(wxNullBitmap);
        Layout();
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

    if (data.empty()) { status_->SetLabel("Failed to download"); return; }

    wxMemoryInputStream mis(data.data(), data.size());
    wxImage img(mis, wxBITMAP_TYPE_ANY);
    if (!img.IsOk()) {
        status_->SetLabel("Failed to decode (" + wxString::Format("%zu", data.size()) + " bytes)");
        return;
    }

    int maxW = bitmap_->GetSize().GetWidth() - 10;
    int maxH = bitmap_->GetSize().GetHeight() - 10;
    if (maxW < 50) maxW = 400;
    if (maxH < 50) maxH = 300;
    if (img.GetWidth() > maxW || img.GetHeight() > maxH) {
        double s = std::min((double)maxW / img.GetWidth(), (double)maxH / img.GetHeight());
        img.Rescale((int)(img.GetWidth() * s), (int)(img.GetHeight() * s), wxIMAGE_QUALITY_HIGH);
    }
    bitmap_->SetBitmap(wxBitmap(img));
    status_->SetLabel(wxString::Format("%dx%d  %.1f KB", img.GetWidth(), img.GetHeight(), data.size() / 1024.0));
    Layout();
}
