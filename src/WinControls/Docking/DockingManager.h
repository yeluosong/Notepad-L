#pragma once
#include "DockPanel.h"
#include <windows.h>
#include <functional>

namespace npp {

enum class DockSide { Left, Right, Bottom };

// Arranges up to 3 dock slots (Left/Right/Bottom) around a central "content"
// region. Each slot has a draggable splitter and a panel title bar with a
// close (X) button. Panels are registered; the manager owns HWNDs for the
// title bar/splitter but does NOT own the panel's own widgets.
class DockingManager
{
public:
    bool Create(HWND parent, HINSTANCE hInst);

    // Register a panel for a side. Call once per side. The panel must be
    // Create()'d by the caller before Register.
    void Register(DockSide side, DockPanel* panel);

    // Replace the panel attached to a slot (keeps the existing title/splitter).
    // The previous panel is hidden; the new one is shown if the slot is visible.
    void SwapPanel(DockSide side, DockPanel* panel);

    // Show/hide a panel. Triggers a relayout.
    void Show(DockSide side, bool show);
    bool IsShown(DockSide side) const;
    DockPanel* Panel(DockSide side) const;

    // Full-area layout. centerOut receives the rect available for the center
    // content (editor). Call from the frame's OnSize.
    void Layout(const RECT& total, RECT& centerOut);

    HWND Hwnd() const { return hwnd_; }

    // Called when a panel's close button is clicked.
    using CloseCallback = std::function<void(DockSide)>;
    void SetOnClose(CloseCallback cb) { onClose_ = std::move(cb); }

private:
    struct Slot {
        DockPanel* panel = nullptr;
        int        size  = 220;   // width (L/R) or height (B) in px
        HWND       title = nullptr;
        HWND       splitter = nullptr;
        RECT       titleRc{};
        RECT       splitRc{};
        bool       visible = false;
    };

    Slot& SlotRef(DockSide s) {
        return s == DockSide::Left ? left_ : s == DockSide::Right ? right_ : bottom_;
    }
    const Slot& SlotRef(DockSide s) const {
        return s == DockSide::Left ? left_ : s == DockSide::Right ? right_ : bottom_;
    }

    void LayoutSlot(DockSide side, Slot& s, const RECT& panelRc);
    static LRESULT CALLBACK TitleProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
    static LRESULT CALLBACK SplitterProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

    HWND      hwnd_   = nullptr; // parent frame
    HINSTANCE hInst_  = nullptr;
    Slot      left_, right_, bottom_;
    CloseCallback onClose_;

    // drag state
    DockSide  dragSide_ = DockSide::Left;
    bool      dragging_ = false;
    POINT     dragStart_{};
    int       dragStartSize_ = 0;
};

} // namespace npp
