#include "Notepad_plus/Notepad_plus_Window.h"
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <string>
#include <vector>

namespace {

// Collect positional (non-switch) args from the command line.
std::vector<std::wstring> CollectFileArgs()
{
    std::vector<std::wstring> files;
    int argc = 0;
    LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (!argv) return files;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg.empty() || arg[0] == L'-' || arg[0] == L'/') continue;
        files.emplace_back(std::move(arg));
    }
    ::LocalFree(argv);
    return files;
}

// If another instance is already running, forward the file list and return
// true; caller should then exit. Paths are joined with '\n'.
bool ForwardToExistingInstance(const std::vector<std::wstring>& files)
{
    HWND target = ::FindWindowW(npp::Notepad_plus_Window::ClassName(), nullptr);
    if (!target) return false;
    // Build a '\n'-joined wchar payload.
    std::wstring payload;
    for (size_t i = 0; i < files.size(); ++i) {
        if (i) payload.push_back(L'\n');
        payload += files[i];
    }
    COPYDATASTRUCT cds{};
    cds.dwData = npp::Notepad_plus_Window::kOpenFilesMsgId;
    cds.cbData = static_cast<DWORD>(payload.size() * sizeof(wchar_t));
    cds.lpData = payload.empty() ? nullptr : payload.data();

    // Let the target take the foreground after it finishes opening.
    DWORD tpid = 0;
    ::GetWindowThreadProcessId(target, &tpid);
    if (tpid) ::AllowSetForegroundWindow(tpid);

    ::SendMessageTimeoutW(target, WM_COPYDATA,
        reinterpret_cast<WPARAM>(::GetForegroundWindow()),
        reinterpret_cast<LPARAM>(&cds),
        SMTO_ABORTIFHUNG, 5000, nullptr);
    return true;
}

} // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInst,
                      _In_opt_ HINSTANCE /*hPrev*/,
                      _In_ LPWSTR /*lpCmdLine*/,
                      _In_ int nCmdShow)
{
    ::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    std::vector<std::wstring> files = CollectFileArgs();

    // Per-user named mutex: single instance within the logged-in session, so
    // multiple users on the same machine each get their own instance.
    HANDLE mutex = ::CreateMutexW(nullptr, FALSE, L"Local\\NotepadPP.SingleInstance.v1");
    const bool alreadyRunning = (::GetLastError() == ERROR_ALREADY_EXISTS);
    if (alreadyRunning) {
        if (ForwardToExistingInstance(files)) {
            if (mutex) ::CloseHandle(mutex);
            ::CoUninitialize();
            return 0;
        }
        // Existing instance not reachable (e.g., still starting up or hidden) —
        // fall through and launch normally rather than lose the user's open.
    }

    npp::Notepad_plus_Window frame;
    frame.SetInitialFiles(std::move(files));
    if (!frame.Init(hInst, nCmdShow)) {
        ::MessageBoxW(nullptr, L"Failed to create main window.",
            L"Notepad++", MB_OK | MB_ICONERROR);
        if (mutex) ::CloseHandle(mutex);
        return 1;
    }

    int rc = frame.MessageLoop();
    if (mutex) ::CloseHandle(mutex);
    ::CoUninitialize();
    return rc;
}
