#include "Parameters.h"
#include "../MISC/Common/FileIO.h"
#include "../MISC/Common/StringUtil.h"

#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <sstream>

namespace npp {

Parameters& Parameters::Instance()
{
    static Parameters p;
    return p;
}

std::wstring Parameters::ConfigDir() const
{
    PWSTR appdata = nullptr;
    std::wstring dir;
    if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appdata))) {
        dir = appdata;
        ::CoTaskMemFree(appdata);
        dir += L"\\NotePad-L";
        ::CreateDirectoryW(dir.c_str(), nullptr);
    }
    return dir;
}

void Parameters::AddRecent(const std::wstring& path)
{
    auto it = std::find_if(recent_.begin(), recent_.end(),
        [&](const std::wstring& p) { return ::_wcsicmp(p.c_str(), path.c_str()) == 0; });
    if (it != recent_.end()) recent_.erase(it);
    recent_.push_front(path);
    while (recent_.size() > kMaxRecent) recent_.pop_back();
}

void Parameters::ClearRecent()
{
    recent_.clear();
}

void Parameters::Load()
{
    const std::wstring file = ConfigDir() + L"\\recent.txt";
    std::vector<char> bytes;
    if (!ReadFileAll(file, bytes)) return;

    std::wstring text = Utf8ToWide(std::string_view(bytes.data(), bytes.size()));
    std::wstringstream ss(text);
    std::wstring line;
    recent_.clear();
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (!line.empty()) recent_.push_back(line);
    }
    while (recent_.size() > kMaxRecent) recent_.pop_back();

    // Prime the save cache so the first Save() after Load() is a no-op when
    // nothing has changed in memory relative to what's already on disk.
    std::wstring primed;
    for (const auto& r : recent_) { primed += r; primed += L"\r\n"; }
    lastSavedBlob_ = WideToUtf8(primed);
}

void Parameters::Save() const
{
    std::wstring text;
    for (const auto& r : recent_) { text += r; text += L"\r\n"; }
    std::string utf8 = WideToUtf8(text);
    if (utf8 == lastSavedBlob_) return;  // nothing changed since last persist
    const std::wstring file = ConfigDir() + L"\\recent.txt";
    if (WriteFileAtomic(file, utf8.data(), utf8.size()))
        lastSavedBlob_ = std::move(utf8);
}

} // namespace npp
