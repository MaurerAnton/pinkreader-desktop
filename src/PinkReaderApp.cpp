#include <wx/wx.h>
#include "PinkReaderApp.h"
#include "MainFrame.h"
#include <curl/curl.h>
#include <cstdio>

wxIMPLEMENT_APP(PinkReaderApp);

bool PinkReaderApp::OnInit() {
    fprintf(stderr, "[pinkreader-desktop] OnInit start\n");
    fflush(stderr);
    curl_global_init(CURL_GLOBAL_ALL);
    fprintf(stderr, "[pinkreader-desktop] curl init done\n");
    fflush(stderr);
    auto *frame = new MainFrame("PinkReader Desktop");
    fprintf(stderr, "[pinkreader-desktop] MainFrame created\n");
    fflush(stderr);
    frame->SetClientSize(1024, 700);
    frame->Center();
    frame->Show();
    fprintf(stderr, "[pinkreader-desktop] frame shown\n");
    fflush(stderr);
    return true;
}

int PinkReaderApp::OnExit() {
    curl_global_cleanup();
    return wxApp::OnExit();
}
