#pragma once
#include <wx/wx.h>
#include <string>

class ImageViewPanel : public wxPanel {
public:
    ImageViewPanel(wxWindow *parent);
    void showImage(const std::string &url, const std::string &caption);

private:
    wxStaticBitmap *bitmap_ = nullptr;
    wxStaticText *caption_ = nullptr;
    wxStaticText *status_ = nullptr;
};
