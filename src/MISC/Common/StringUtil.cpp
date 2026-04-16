#include "StringUtil.h"
#include <windows.h>

namespace npp {

std::string WideToUtf8(std::wstring_view w)
{
    if (w.empty()) return {};
    const int n = ::WideCharToMultiByte(CP_UTF8, 0,
        w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()),
        s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring Utf8ToWide(std::string_view u)
{
    if (u.empty()) return {};
    const int n = ::MultiByteToWideChar(CP_UTF8, 0,
        u.data(), static_cast<int>(u.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, u.data(), static_cast<int>(u.size()),
        w.data(), n);
    return w;
}

std::wstring PathFileName(std::wstring_view fullPath)
{
    const size_t pos = fullPath.find_last_of(L"\\/");
    return (pos == std::wstring_view::npos)
        ? std::wstring(fullPath)
        : std::wstring(fullPath.substr(pos + 1));
}

} // namespace npp
