#include "DocMapPanel.h"
#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <commctrl.h>
#include <windowsx.h>

namespace npp {

extern "C" int Scintilla_RegisterClasses(void* hInstance);

HWND DocMapPanel::Create(HWND parent, HINSTANCE hInst)
{
    Scintilla_RegisterClasses(hInst);
    // Container.
    hwnd_ = ::CreateWindowExW(
        WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 0, 0, parent, nullptr, hInst, nullptr);
    sci_ = ::CreateWindowExW(
        0, L"Scintilla", L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 10, 10, hwnd_, nullptr, hInst, nullptr);
    directPtr_ = static_cast<sptr_t>(::SendMessageW(sci_, SCI_GETDIRECTPOINTER, 0, 0));
    direct_    = reinterpret_cast<DirectFn>(::SendMessageW(sci_, SCI_GETDIRECTFUNCTION, 0, 0));

    // Mini-view styling: tiny font, no caret visible, read-only once doc attached.
    Call(SCI_SETCODEPAGE, SC_CP_UTF8);
    Call(SCI_STYLESETSIZE, STYLE_DEFAULT, 2);
    Call(SCI_STYLECLEARALL);
    Call(SCI_SETMARGINWIDTHN, 0, 0);
    Call(SCI_SETMARGINWIDTHN, 1, 0);
    Call(SCI_SETMARGINWIDTHN, 2, 0);
    Call(SCI_SETHSCROLLBAR, 0);
    Call(SCI_SETVSCROLLBAR, 0);
    Call(SCI_SETCARETLINEVISIBLE, 0);
    Call(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
    Call(SCI_SETWRAPMODE, SC_WRAP_WORD);

    ::SetWindowLongPtrW(sci_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SetWindowSubclass(sci_, &DocMapPanel::ProxyProc, 1, 0);
    return hwnd_;
}

void DocMapPanel::Resize(const RECT& inner)
{
    ::SetWindowPos(sci_, nullptr, 0, 0,
        inner.right - inner.left, inner.bottom - inner.top,
        SWP_NOZORDER);
}

sptr_t DocMapPanel::Call(unsigned int msg, uptr_t w, sptr_t l) const
{
    if (direct_) return direct_(directPtr_, msg, w, l);
    return ::SendMessageW(sci_, msg, w, l);
}

void DocMapPanel::AttachDoc(sptr_t docHandle)
{
    if (!sci_) return;
    Call(SCI_SETDOCPOINTER, 0, docHandle);
    Call(SCI_SETREADONLY, 1);
}

void DocMapPanel::UpdateViewport(int firstVisibleLine, int linesOnScreen)
{
    firstVis_ = firstVisibleLine;
    linesOnScreen_ = linesOnScreen;
    if (!sci_) return;
    Call(SCI_GOTOLINE, firstVisibleLine);
    ::InvalidateRect(sci_, nullptr, FALSE);
}

LRESULT CALLBACK DocMapPanel::ProxyProc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR, DWORD_PTR)
{
    auto* self = reinterpret_cast<DocMapPanel*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_LBUTTONDOWN && self) {
        int y = GET_Y_LPARAM(l);
        sptr_t line = self->Call(SCI_LINEFROMPOSITION,
            self->Call(SCI_POSITIONFROMPOINT, 0, y));
        int targetFirst = static_cast<int>(line) - self->linesOnScreen_ / 2;
        if (targetFirst < 0) targetFirst = 0;
        if (self->onScroll_) self->onScroll_(targetFirst);
        return 0;
    }
    if (m == WM_PAINT && self) {
        LRESULT r = ::DefSubclassProc(h, m, w, l);
        // Overlay viewport rectangle.
        HDC hdc = ::GetDC(h);
        RECT rc; ::GetClientRect(h, &rc);
        sptr_t posStart = self->Call(SCI_POSITIONFROMLINE, self->firstVis_);
        sptr_t posEnd   = self->Call(SCI_POSITIONFROMLINE,
            self->firstVis_ + self->linesOnScreen_);
        int yTop    = static_cast<int>(self->Call(SCI_POINTYFROMPOSITION, 0, posStart));
        int yBottom = (posEnd >= 0)
            ? static_cast<int>(self->Call(SCI_POINTYFROMPOSITION, 0, posEnd))
            : rc.bottom;
        if (yBottom <= yTop) yBottom = yTop + 10;
        RECT box{ rc.left, yTop, rc.right, yBottom };
        HBRUSH fill = ::CreateSolidBrush(RGB(180, 200, 255));
        HGDIOBJ oldBrush = ::SelectObject(hdc, fill);
        // Light semi-transparent-ish fill via pattern; FrameRect for clarity.
        ::FrameRect(hdc, &box, fill);
        // Double-line border.
        InflateRect(&box, -1, -1);
        ::FrameRect(hdc, &box, fill);
        ::SelectObject(hdc, oldBrush);
        ::DeleteObject(fill);
        ::ReleaseDC(h, hdc);
        return r;
    }
    return ::DefSubclassProc(h, m, w, l);
}

} // namespace npp
