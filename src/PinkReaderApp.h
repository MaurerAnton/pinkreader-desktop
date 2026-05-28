#pragma once
#include <wx/wx.h>

class PinkReaderApp : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;
};

wxDECLARE_APP(PinkReaderApp);
