#include "Buffer.h"
#include "../MISC/Common/StringUtil.h"
#include <string>

namespace npp {

Buffer::Buffer(BufferID id, sptr_t docHandle, std::wstring path,
               bool untitled, int untitledIndex)
    : id_(id)
    , docHandle_(docHandle)
    , path_(std::move(path))
    , untitled_(untitled)
    , untitledIndex_(untitledIndex)
{}

std::wstring Buffer::DisplayName() const
{
    if (untitled_) return L"new " + std::to_wstring(untitledIndex_);
    return PathFileName(path_);
}

void Buffer::SetPath(const std::wstring& newPath)
{
    path_ = newPath;
    untitled_ = newPath.empty();
}

} // namespace npp
