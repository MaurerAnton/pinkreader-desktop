#pragma once
#include <wx/wx.h>
#include <string>

class ImageViewPanel : public wxPanel {
public:
    ImageViewPanel(wxWindow *parent);
    ~ImageViewPanel();
    void showImage(const std::string &url, const std::string &caption);
    void showVideoInfo(const std::string &url, const std::string &caption);
    void showGallery(const std::vector<std::string> &urls, const std::string &caption);

private:
    void onPlayClick(wxCommandEvent &evt);
    void onPrevClick(wxCommandEvent &evt);
    void onNextClick(wxCommandEvent &evt);
    void loadGalleryImage(int idx);
    std::string lastVideoUrl_;
    std::string pendingVideoUrl_;
    std::vector<std::string> galleryUrls_;
    int galleryIdx_ = 0;

    wxStaticBitmap *bitmap_ = nullptr;
    wxPanel *videoPanel_ = nullptr;
    wxStaticText *caption_ = nullptr;
    wxStaticText *status_ = nullptr;
    wxButton *playBtn_ = nullptr;
    wxButton *prevBtn_ = nullptr;
    wxButton *nextBtn_ = nullptr;
    long mpvPid_ = 0;
    void killMpv();
};
