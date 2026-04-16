#pragma once
#include "DockPanel.h"
#include <windows.h>
#include <cstdint>
#include <functional>

typedef intptr_t sptr_t;
typedef uintptr_t uptr_t;

namespace npp {

// Right-dock miniature overview. A second Scintilla window attached to the
// same document handle as the active editor, shown at a tiny zoom. Supports
// click-to-scroll and highlights the main viewport rectangle.
class DocMapPanel : public DockPanel
{
public:
    HWND Create(HWND parent, HINSTANCE hInst) override;
    const wchar_t* Title() const override { return L"Document Map"; }
    void Resize(const RECT& inner) override;

    // Attach/detach the active document (SCI_SETDOCPOINTER).
    void AttachDoc(sptr_t docHandle);

    // Called on editor scroll/resize — updates the viewport highlight lines.
    void UpdateViewport(int firstVisibleLine, int linesOnScreen);

    // Wire back a scroll request — caller navigates the main editor.
    using ScrollCb = std::function<void(int targetFirstVisibleLine)>;
    void SetOnScroll(ScrollCb cb) { onScroll_ = std::move(cb); }

    HWND Sci() const { return sci_; }

private:
    static LRESULT CALLBACK ProxyProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    sptr_t Call(unsigned int msg, uptr_t w = 0, sptr_t l = 0) const;

    HWND sci_ = nullptr;
    using DirectFn = sptr_t (*)(sptr_t, unsigned int, uptr_t, sptr_t);
    DirectFn direct_ = nullptr;
    sptr_t   directPtr_ = 0;

    int firstVis_ = 0, linesOnScreen_ = 0;
    ScrollCb onScroll_;
};

} // namespace npp
