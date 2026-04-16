#include "DockingManager.h"
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>

namespace npp {

namespace {
constexpr int kTitleH    = 22;
constexpr int kSplitW    = 4;
constexpr int kCloseSize = 14;

const wchar_t kTitleClass[]    = L"NotepadPP_DockTitle";
const wchar_t kSplitterClass[] = L"NotepadPP_DockSplit";

bool g_classesRegistered = false;

void EnsureClasses(HINSTANCE hInst)
{
    if (g_classesRegistered) return;
    WNDCLASSW wc{};
    wc.lpfnWndProc   = ::DefWindowProcW;
    wc.hInstance     = hInst;
    wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
    wc.lpszClassName = kTitleClass;
    ::RegisterClassW(&wc);

    WNDCLASSW wc2{};
    wc2.lpfnWndProc   = ::DefWindowProcW;
    wc2.hInstance     = hInst;
    wc2.hbrBackground = ::GetSysColorBrush(COLOR_3DSHADOW);
    wc2.lpszClassName = kSplitterClass;
    ::RegisterClassW(&wc2);

    g_classesRegistered = true;
}

LRESULT CALLBACK ForwardProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR)
{
    if (m == WM_NOTIFY) {
        return ::SendMessageW(::GetParent(h), WM_NOTIFY, w, l);
    }
    return ::DefSubclassProc(h, m, w, l);
}

} // namespace

namespace dockutil {
void ForwardNotifyToParent(HWND container)
{
    ::SetWindowSubclass(container, &ForwardProc, 0xDC01, 0);
}
} // namespace dockutil

bool DockingManager::Create(HWND parent, HINSTANCE hInst)
{
    hwnd_ = parent;
    hInst_ = hInst;
    EnsureClasses(hInst);
    return true;
}

void DockingManager::Register(DockSide side, DockPanel* panel)
{
    Slot& s = SlotRef(side);
    s.panel = panel;
    s.visible = panel ? panel->Visible() : false;

    // Create title bar.
    s.title = ::CreateWindowExW(
        0, kTitleClass, panel ? panel->Title() : L"",
        WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);
    ::SetWindowLongPtrW(s.title, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SetWindowSubclass(s.title, &DockingManager::TitleProc, static_cast<UINT_PTR>(side), 0);

    // Splitter.
    HCURSOR cur = (side == DockSide::Bottom) ? ::LoadCursorW(nullptr, IDC_SIZENS)
                                             : ::LoadCursorW(nullptr, IDC_SIZEWE);
    s.splitter = ::CreateWindowExW(
        0, kSplitterClass, L"",
        WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, 0, 0, hwnd_, nullptr, hInst_, nullptr);
    ::SetClassLongPtrW(s.splitter, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cur));
    ::SetWindowLongPtrW(s.splitter, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ::SetWindowSubclass(s.splitter, &DockingManager::SplitterProc, static_cast<UINT_PTR>(side), 0);
}

void DockingManager::SwapPanel(DockSide side, DockPanel* panel)
{
    Slot& s = SlotRef(side);
    if (s.panel && s.panel->Hwnd()) ::ShowWindow(s.panel->Hwnd(), SW_HIDE);
    s.panel = panel;
    if (s.title && panel) ::SetWindowTextW(s.title, panel->Title());
    if (panel && panel->Hwnd() && s.visible) ::ShowWindow(panel->Hwnd(), SW_SHOW);
}

void DockingManager::Show(DockSide side, bool show)
{
    Slot& s = SlotRef(side);
    s.visible = show;
    if (s.panel) s.panel->SetVisible(show);
    int cmd = show ? SW_SHOW : SW_HIDE;
    if (s.title)    ::ShowWindow(s.title, cmd);
    if (s.splitter) ::ShowWindow(s.splitter, cmd);
    if (s.panel && s.panel->Hwnd()) ::ShowWindow(s.panel->Hwnd(), cmd);
}

bool DockingManager::IsShown(DockSide side) const
{
    return SlotRef(side).visible;
}

DockPanel* DockingManager::Panel(DockSide side) const
{
    return SlotRef(side).panel;
}

void DockingManager::Layout(const RECT& total, RECT& centerOut)
{
    RECT rc = total;

    // Bottom reserved first (it crosses full width including under L/R).
    if (bottom_.visible && bottom_.panel) {
        int h = std::min<int>(bottom_.size, (rc.bottom - rc.top) - 100);
        if (h < 60) h = 60;
        RECT panelRc{ rc.left, rc.bottom - h, rc.right, rc.bottom };
        rc.bottom -= h;
        // Splitter just above bottom panel.
        RECT splitRc{ rc.left, rc.bottom - kSplitW, rc.right, rc.bottom };
        rc.bottom -= kSplitW;
        ::SetWindowPos(bottom_.splitter, nullptr, splitRc.left, splitRc.top,
            splitRc.right - splitRc.left, splitRc.bottom - splitRc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        bottom_.splitRc = splitRc;
        LayoutSlot(DockSide::Bottom, bottom_, panelRc);
    }

    if (left_.visible && left_.panel) {
        int w = std::min<int>(left_.size, (rc.right - rc.left) - 200);
        if (w < 80) w = 80;
        RECT panelRc{ rc.left, rc.top, rc.left + w, rc.bottom };
        rc.left += w;
        RECT splitRc{ rc.left, rc.top, rc.left + kSplitW, rc.bottom };
        rc.left += kSplitW;
        ::SetWindowPos(left_.splitter, nullptr, splitRc.left, splitRc.top,
            splitRc.right - splitRc.left, splitRc.bottom - splitRc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        left_.splitRc = splitRc;
        LayoutSlot(DockSide::Left, left_, panelRc);
    }

    if (right_.visible && right_.panel) {
        int w = std::min<int>(right_.size, (rc.right - rc.left) - 200);
        if (w < 80) w = 80;
        RECT panelRc{ rc.right - w, rc.top, rc.right, rc.bottom };
        rc.right -= w;
        RECT splitRc{ rc.right - kSplitW, rc.top, rc.right, rc.bottom };
        rc.right -= kSplitW;
        ::SetWindowPos(right_.splitter, nullptr, splitRc.left, splitRc.top,
            splitRc.right - splitRc.left, splitRc.bottom - splitRc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        right_.splitRc = splitRc;
        LayoutSlot(DockSide::Right, right_, panelRc);
    }

    centerOut = rc;
}

void DockingManager::LayoutSlot(DockSide /*side*/, Slot& s, const RECT& panelRc)
{
    RECT titleRc{ panelRc.left, panelRc.top,
                  panelRc.right, panelRc.top + kTitleH };
    RECT innerRc{ panelRc.left, panelRc.top + kTitleH,
                  panelRc.right, panelRc.bottom };
    s.titleRc = titleRc;
    ::SetWindowPos(s.title, nullptr, titleRc.left, titleRc.top,
        titleRc.right - titleRc.left, titleRc.bottom - titleRc.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ::InvalidateRect(s.title, nullptr, TRUE);
    if (s.panel && s.panel->Hwnd()) {
        ::SetWindowPos(s.panel->Hwnd(), nullptr, innerRc.left, innerRc.top,
            innerRc.right - innerRc.left, innerRc.bottom - innerRc.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        s.panel->Resize(innerRc);
    }
}

LRESULT CALLBACK DockingManager::TitleProc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR id, DWORD_PTR)
{
    auto* self = reinterpret_cast<DockingManager*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    DockSide side = static_cast<DockSide>(id);
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(h, &ps);
        RECT rc; ::GetClientRect(h, &rc);
        ::FillRect(hdc, &rc, ::GetSysColorBrush(COLOR_BTNFACE));
        // Bottom border.
        HPEN p = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_3DSHADOW));
        HGDIOBJ op = ::SelectObject(hdc, p);
        ::MoveToEx(hdc, rc.left, rc.bottom - 1, nullptr);
        ::LineTo(hdc, rc.right, rc.bottom - 1);
        ::SelectObject(hdc, op);
        ::DeleteObject(p);
        // Title text.
        wchar_t buf[128]{};
        ::GetWindowTextW(h, buf, 127);
        ::SetBkMode(hdc, TRANSPARENT);
        RECT tx = rc; tx.left += 6; tx.right -= (kCloseSize + 8);
        ::DrawTextW(hdc, buf, -1, &tx, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
        // Close button.
        RECT cb{ rc.right - kCloseSize - 4, (rc.bottom - kCloseSize) / 2,
                 rc.right - 4, (rc.bottom - kCloseSize) / 2 + kCloseSize };
        ::FrameRect(hdc, &cb, ::GetSysColorBrush(COLOR_3DSHADOW));
        ::MoveToEx(hdc, cb.left + 3, cb.top + 3, nullptr);
        ::LineTo(hdc, cb.right - 3, cb.bottom - 3);
        ::MoveToEx(hdc, cb.right - 4, cb.top + 3, nullptr);
        ::LineTo(hdc, cb.left + 2, cb.bottom - 3);
        ::EndPaint(h, &ps);
        return 0;
    }
    case WM_LBUTTONUP: {
        RECT rc; ::GetClientRect(h, &rc);
        int x = GET_X_LPARAM(l), y = GET_Y_LPARAM(l);
        RECT cb{ rc.right - kCloseSize - 4, (rc.bottom - kCloseSize) / 2,
                 rc.right - 4, (rc.bottom - kCloseSize) / 2 + kCloseSize };
        if (x >= cb.left && x < cb.right && y >= cb.top && y < cb.bottom) {
            if (self && self->onClose_) self->onClose_(side);
        }
        return 0;
    }
    }
    return ::DefSubclassProc(h, m, w, l);
}

LRESULT CALLBACK DockingManager::SplitterProc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR id, DWORD_PTR)
{
    auto* self = reinterpret_cast<DockingManager*>(::GetWindowLongPtrW(h, GWLP_USERDATA));
    DockSide side = static_cast<DockSide>(id);
    switch (m) {
    case WM_LBUTTONDOWN: {
        ::SetCapture(h);
        self->dragging_ = true;
        self->dragSide_ = side;
        POINT p{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        ::ClientToScreen(h, &p);
        self->dragStart_ = p;
        self->dragStartSize_ = self->SlotRef(side).size;
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (!self->dragging_) break;
        POINT p{ GET_X_LPARAM(l), GET_Y_LPARAM(l) };
        ::ClientToScreen(h, &p);
        int delta = 0;
        if (side == DockSide::Left)        delta =  p.x - self->dragStart_.x;
        else if (side == DockSide::Right)  delta = -(p.x - self->dragStart_.x);
        else                                delta = -(p.y - self->dragStart_.y);
        int ns = self->dragStartSize_ + delta;
        if (ns < 80) ns = 80;
        if (ns > 900) ns = 900;
        self->SlotRef(side).size = ns;
        RECT client; ::GetClientRect(self->hwnd_, &client);
        ::SendMessageW(self->hwnd_, WM_SIZE, 0, 0);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (self->dragging_) {
            self->dragging_ = false;
            ::ReleaseCapture();
        }
        return 0;
    }
    }
    return ::DefSubclassProc(h, m, w, l);
}

} // namespace npp
