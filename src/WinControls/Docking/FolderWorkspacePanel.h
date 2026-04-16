#pragma once
#include "DockPanel.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <unordered_map>
#include <functional>

namespace npp {

// Left-dock folder tree. Lazy-expand subdirectories.
class FolderWorkspacePanel : public DockPanel
{
public:
    HWND Create(HWND parent, HINSTANCE hInst) override;
    const wchar_t* Title() const override { return L"Folder as Workspace"; }
    void Resize(const RECT& inner) override;

    // Add a top-level folder root.
    void AddRoot(const std::wstring& path);

    using OpenCb = std::function<void(const std::wstring& path)>;
    void SetOnOpen(OpenCb cb) { onOpen_ = std::move(cb); }

    LRESULT HandleNotify(LPARAM lParam);

private:
    HWND tree_ = nullptr;
    std::unordered_map<HTREEITEM, std::wstring> pathMap_;
    OpenCb onOpen_;

    HTREEITEM InsertItem(HTREEITEM parent, const std::wstring& label,
                         const std::wstring& fullPath, bool isFolder);
    void LoadChildren(HTREEITEM parent);
    bool IsDummyChild(HTREEITEM parent) const;
};

} // namespace npp
