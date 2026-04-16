#pragma once
#include <windows.h>
#include <string>

namespace npp {

struct FindInFilesParams {
    std::wstring dir;
    std::wstring filters = L"*.*";
    std::wstring what;
    bool matchCase = false;
    bool wholeWord = false;
    bool regex     = false;
    bool subdirs   = true;
};

// Shows a modal "Find in Files" dialog built from an in-memory DLGTEMPLATE.
// Returns true if the user clicked "Find All"; out receives the parameters.
// `defaults` seeds the initial values.
bool ShowFindInFilesDlg(HWND owner, HINSTANCE hInst,
                        const FindInFilesParams& defaults,
                        FindInFilesParams& out);

} // namespace npp
