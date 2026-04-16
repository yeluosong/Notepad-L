#include "FindResultsPanel.h"
#include <commctrl.h>
#include <string>

namespace npp {

HWND FindResultsPanel::Create(HWND parent, HINSTANCE hInst)
{
    hwnd_ = ::CreateWindowExW(
        WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 0, 0, parent, nullptr, hInst, nullptr);

    list_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, 10, 10, hwnd_, nullptr, hInst, nullptr);
    ListView_SetExtendedListViewStyle(list_,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    dockutil::ForwardNotifyToParent(hwnd_);

    LVCOLUMNW c{};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = const_cast<LPWSTR>(L"File");     c.cx = 260;
    ListView_InsertColumn(list_, 0, &c);
    c.pszText = const_cast<LPWSTR>(L"Line");     c.cx = 60;
    ListView_InsertColumn(list_, 1, &c);
    c.pszText = const_cast<LPWSTR>(L"Text");     c.cx = 600;
    ListView_InsertColumn(list_, 2, &c);
    return hwnd_;
}

void FindResultsPanel::Resize(const RECT& inner)
{
    if (!hwnd_) return;
    // hwnd_ already sized by DockingManager. Fit list to inner-local coords.
    int w = inner.right - inner.left;
    int h = inner.bottom - inner.top;
    ::SetWindowPos(list_, nullptr, 0, 0, w, h, SWP_NOZORDER);
}

void FindResultsPanel::Clear()
{
    hits_.clear();
    summary_.clear();
    ListView_DeleteAllItems(list_);
}

void FindResultsPanel::SetSummary(const std::wstring& s)
{
    summary_ = s;
    // Insert at index 0 as a visual hint; mark line=-1 (no jump).
    LVITEMW it{};
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = 0;
    it.lParam = -1;
    std::wstring head = L"[ " + s + L" ]";
    it.pszText = const_cast<LPWSTR>(head.c_str());
    ListView_InsertItem(list_, &it);
    ListView_SetItemText(list_, 0, 1, const_cast<LPWSTR>(L""));
    ListView_SetItemText(list_, 0, 2, const_cast<LPWSTR>(L""));
}

void FindResultsPanel::AddHit(const FindHit& h)
{
    hits_.push_back(h);
    int idx = static_cast<int>(hits_.size()) - 1;   // stored index
    LVITEMW it{};
    it.mask = LVIF_TEXT | LVIF_PARAM;
    it.iItem = ListView_GetItemCount(list_);
    it.lParam = idx;
    it.pszText = const_cast<LPWSTR>(hits_[idx].path.c_str());
    int row = ListView_InsertItem(list_, &it);
    wchar_t ln[16];
    _snwprintf_s(ln, 16, _TRUNCATE, L"%d", h.line);
    ListView_SetItemText(list_, row, 1, ln);
    ListView_SetItemText(list_, row, 2, const_cast<LPWSTR>(hits_[idx].text.c_str()));
}

LRESULT FindResultsPanel::HandleNotify(LPARAM lParam)
{
    auto* nm = reinterpret_cast<LPNMHDR>(lParam);
    if (!nm || nm->hwndFrom != list_) return 0;
    if (nm->code == NM_DBLCLK || nm->code == NM_RETURN) {
        int sel = ListView_GetNextItem(list_, -1, LVNI_SELECTED);
        if (sel < 0) return 0;
        LVITEMW it{}; it.iItem = sel; it.mask = LVIF_PARAM;
        ListView_GetItem(list_, &it);
        if (it.lParam < 0) return 0;
        size_t i = static_cast<size_t>(it.lParam);
        if (i < hits_.size() && onGoto_) onGoto_(hits_[i].path, hits_[i].line);
    }
    return 0;
}

} // namespace npp
