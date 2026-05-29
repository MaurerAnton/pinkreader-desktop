#pragma once
#include <wx/wx.h>
#include <wx/mediactrl.h>
#include <string>

class ImageViewPanel : public wxPanel {
public:
    ImageViewPanel(wxWindow *parent);
    void showImage(const std::string &url, const std::string &caption);
    void showVideoInfo(const std::string &url, const std::string &caption);

private:
    void onPlayClick(wxCommandEvent &evt);
    void onMediaLoaded(wxMediaEvent &evt);
    std::string lastVideoUrl_;
    std::string pendingVideoUrl_;

    wxStaticBitmap *bitmap_ = nullptr;
    wxMediaCtrl *media_ = nullptr;
    wxStaticText *caption_ = nullptr;
    wxStaticText *status_ = nullptr;
    wxButton *playBtn_ = nullptr;
    bool videoLoaded_ = false;
};
