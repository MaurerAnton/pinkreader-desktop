#pragma once
#include <wx/wx.h>
#include <string>

class ImageViewPanel : public wxPanel {
public:
    ImageViewPanel(wxWindow *parent);
    ~ImageViewPanel();
    void showImage(const std::string &url, const std::string &caption);
    void showVideoInfo(const std::string &url, const std::string &caption);

private:
    void onPlayClick(wxCommandEvent &evt);
    std::string lastVideoUrl_;
    std::string pendingVideoUrl_;

    wxStaticBitmap *bitmap_ = nullptr;
    wxPanel *videoPanel_ = nullptr;
    wxStaticText *caption_ = nullptr;
    wxStaticText *status_ = nullptr;
    wxButton *playBtn_ = nullptr;
    long mpvPid_ = 0;
    void killMpv();
};
