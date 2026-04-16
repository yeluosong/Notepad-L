#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace npp {

// Reads entire file into bytes. Returns false on failure.
bool ReadFileAll(const std::wstring& path, std::vector<char>& out);

// Writes bytes atomically (temp file + MoveFileEx replace).
bool WriteFileAtomic(const std::wstring& path, const char* data, size_t size);

} // namespace npp
