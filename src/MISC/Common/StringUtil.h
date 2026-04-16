#pragma once
#include <string>
#include <string_view>

namespace npp {

std::string  WideToUtf8(std::wstring_view w);
std::wstring Utf8ToWide(std::string_view  u);

// Returns the file name portion of a path (no drive, no directory).
std::wstring PathFileName(std::wstring_view fullPath);

} // namespace npp
