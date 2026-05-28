#include "PinkReaderApp.h"
#include "MainFrame.h"
#include <curl/curl.h>

wxIMPLEMENT_APP(PinkReaderApp);

bool PinkReaderApp::OnInit() {
    curl_global_init(CURL_GLOBAL_ALL);
    auto *frame = new MainFrame("PinkReader Desktop");
    frame->SetClientSize(1024, 700);
    frame->Center();
    frame->Show();
    return true;
}

int PinkReaderApp::OnExit() {
    curl_global_cleanup();
    return wxApp::OnExit();
}
