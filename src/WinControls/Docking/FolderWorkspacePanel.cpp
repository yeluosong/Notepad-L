#include "FolderWorkspacePanel.h"
#include "../../MISC/Common/StringUtil.h"

#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <algorithm>

namespace npp {

HWND FolderWorkspacePanel::Create(HWND parent, HINSTANCE hInst)
{
    hwnd_ = ::CreateWindowExW(
        WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 0, 0, parent, nullptr, hInst, nullptr);
    tree_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT |
        TVS_SHOWSELALWAYS,
        0, 0, 10, 10, hwnd_, nullptr, hInst, nullptr);
    dockutil::ForwardNotifyToParent(hwnd_);
    return hwnd_;
}

void FolderWorkspacePanel::Resize(const RECT& inner)
{
    ::SetWindowPos(tree_, nullptr, 0, 0,
        inner.right - inner.left, inner.bottom - inner.top,
        SWP_NOZORDER);
}

HTREEITEM FolderWorkspacePanel::InsertItem(HTREEITEM parent, const std::wstring& label,
                                            const std::wstring& fullPath, bool isFolder)
{
    TVINSERTSTRUCTW ti{};
    ti.hParent = parent;
    ti.hInsertAfter = TVI_SORT;
    ti.item.mask = TVIF_TEXT | TVIF_CHILDREN;
    ti.item.pszText = const_cast<LPWSTR>(label.c_str());
    ti.item.cChildren = isFolder ? 1 : 0;
    HTREEITEM h = TreeView_InsertItem(tree_, &ti);
    pathMap_[h] = fullPath;
    if (isFolder) {
        // Insert dummy child so the [+] appears.
        TVINSERTSTRUCTW d{};
        d.hParent = h; d.hInsertAfter = TVI_LAST;
        d.item.mask = TVIF_TEXT;
        d.item.pszText = const_cast<LPWSTR>(L"");
        TreeView_InsertItem(tree_, &d);
    }
    return h;
}

void FolderWorkspacePanel::AddRoot(const std::wstring& path)
{
    std::wstring label = path;
    InsertItem(TVI_ROOT, label, path, true);
}

bool FolderWorkspacePanel::IsDummyChild(HTREEITEM parent) const
{
    HTREEITEM c = TreeView_GetChild(tree_, parent);
    if (!c) return false;
    wchar_t buf[4]{};
    TVITEMW it{}; it.mask = TVIF_TEXT; it.hItem = c;
    it.pszText = buf; it.cchTextMax = 3;
    TreeView_GetItem(tree_, &it);
    return buf[0] == 0 && TreeView_GetNextSibling(tree_, c) == nullptr;
}

void FolderWorkspacePanel::LoadChildren(HTREEITEM parent)
{
    auto it = pathMap_.find(parent);
    if (it == pathMap_.end()) return;
    std::wstring dir = it->second;

    // Remove dummy.
    HTREEITEM c = TreeView_GetChild(tree_, parent);
    while (c) {
        HTREEITEM next = TreeView_GetNextSibling(tree_, c);
        pathMap_.erase(c);
        TreeView_DeleteItem(tree_, c);
        c = next;
    }

    WIN32_FIND_DATAW fd;
    std::wstring pattern = dir + L"\\*";
    HANDLE hf = ::FindFirstFileW(pattern.c_str(), &fd);
    if (hf == INVALID_HANDLE_VALUE) return;

    std::vector<std::pair<std::wstring, bool>> entries;
    do {
        if (fd.cFileName[0] == L'.' &&
            (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
            continue;
        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entries.emplace_back(fd.cFileName, isDir);
    } while (::FindNextFileW(hf, &fd));
    ::FindClose(hf);

    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            if (a.second != b.second) return a.second > b.second;   // folders first
            return _wcsicmp(a.first.c_str(), b.first.c_str()) < 0;
        });

    for (const auto& e : entries) {
        InsertItem(parent, e.first, dir + L"\\" + e.first, e.second);
    }
}

LRESULT FolderWorkspacePanel::HandleNotify(LPARAM lParam)
{
    auto* nm = reinterpret_cast<LPNMHDR>(lParam);
    if (!nm || nm->hwndFrom != tree_) return 0;
    if (nm->code == TVN_ITEMEXPANDINGW) {
        auto* ex = reinterpret_cast<LPNMTREEVIEWW>(lParam);
        if (ex->action == TVE_EXPAND && IsDummyChild(ex->itemNew.hItem)) {
            LoadChildren(ex->itemNew.hItem);
        }
        return 0;
    }
    if (nm->code == NM_DBLCLK) {
        HTREEITEM sel = TreeView_GetSelection(tree_);
        if (!sel) return 0;
        auto it = pathMap_.find(sel);
        if (it == pathMap_.end()) return 0;
        DWORD attr = ::GetFileAttributesW(it->second.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            if (onOpen_) onOpen_(it->second);
        }
    }
    return 0;
}

} // namespace npp
