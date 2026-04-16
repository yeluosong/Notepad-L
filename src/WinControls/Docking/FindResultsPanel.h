#pragma once
#include "DockPanel.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace npp {

struct FindHit {
    std::wstring path;
    int          line = 0;   // 1-based
    std::wstring text;       // line text (trimmed)
};

// Bottom-dock ListView showing Find-in-Files hits. Double-click a row fires
// the callback with (path, line).
class FindResultsPanel : public DockPanel
{
public:
    HWND Create(HWND parent, HINSTANCE hInst) override;
    const wchar_t* Title() const override { return L"Find Results"; }
    void Resize(const RECT& inner) override;

    void Clear();
    void AddHit(const FindHit& h);
    void SetSummary(const std::wstring& s);   // shown as first "(summary)" row
    size_t HitCount() const { return hits_.size(); }

    using GotoCb = std::function<void(const std::wstring& path, int line)>;
    void SetOnGoto(GotoCb cb) { onGoto_ = std::move(cb); }

    // Forward WM_NOTIFY from parent.
    LRESULT HandleNotify(LPARAM lParam);

private:
    HWND                  list_ = nullptr;
    std::vector<FindHit>  hits_;
    std::wstring          summary_;
    GotoCb                onGoto_;
};

} // namespace npp
