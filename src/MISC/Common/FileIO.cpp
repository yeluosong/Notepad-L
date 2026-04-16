#include "FileIO.h"
#include <windows.h>

namespace npp {

bool ReadFileAll(const std::wstring& path, std::vector<char>& out)
{
    HANDLE h = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER sz{};
    if (!::GetFileSizeEx(h, &sz) || sz.QuadPart < 0) { ::CloseHandle(h); return false; }

    out.resize(static_cast<size_t>(sz.QuadPart));
    size_t remaining = out.size();
    char* p = out.data();
    while (remaining > 0) {
        DWORD toRead = static_cast<DWORD>(
            remaining > 0x10000000 ? 0x10000000 : remaining);
        DWORD got = 0;
        if (!::ReadFile(h, p, toRead, &got, nullptr) || got == 0) {
            ::CloseHandle(h);
            return false;
        }
        p += got;
        remaining -= got;
    }
    ::CloseHandle(h);
    return true;
}

bool WriteFileAtomic(const std::wstring& path, const char* data, size_t size)
{
    std::wstring tmp = path + L".npp.tmp";
    HANDLE h = ::CreateFileW(tmp.c_str(), GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    const char* p = data;
    size_t remaining = size;
    while (remaining > 0) {
        DWORD toWrite = static_cast<DWORD>(
            remaining > 0x10000000 ? 0x10000000 : remaining);
        DWORD wrote = 0;
        if (!::WriteFile(h, p, toWrite, &wrote, nullptr) || wrote == 0) {
            ::CloseHandle(h);
            ::DeleteFileW(tmp.c_str());
            return false;
        }
        p += wrote;
        remaining -= wrote;
    }
    ::FlushFileBuffers(h);
    ::CloseHandle(h);

    if (!::MoveFileExW(tmp.c_str(), path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        ::DeleteFileW(tmp.c_str());
        return false;
    }
    return true;
}

} // namespace npp
