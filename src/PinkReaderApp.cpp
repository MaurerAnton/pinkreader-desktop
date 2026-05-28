#include "PinkReaderApp.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(PinkReaderApp);

bool PinkReaderApp::OnInit() {
    auto *frame = new MainFrame("PinkReader Desktop");
    frame->SetClientSize(1024, 700);
    frame->Center();
    frame->Show();
    return true;
}
