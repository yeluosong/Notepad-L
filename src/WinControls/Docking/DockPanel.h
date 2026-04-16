#pragma once
#include <windows.h>
#include <string>

namespace npp {

// Minimal base class for a dockable side panel. Each panel owns a single HWND
// (its "root" — typically a container into which the real content is built).
class DockPanel
{
public:
    virtual ~DockPanel() = default;

    // Create the panel's root window as a child of `parent`. Return the root
    // HWND (also stored in hwnd_). Called once.
    virtual HWND Create(HWND parent, HINSTANCE hInst) = 0;

    // Caption shown in the panel title bar.
    virtual const wchar_t* Title() const = 0;

    // Resize the content area given the panel's inner client rect (title
    // bar already subtracted by DockingManager).
    virtual void Resize(const RECT& inner) = 0;

    HWND Hwnd() const { return hwnd_; }
    bool Visible() const { return visible_; }
    void SetVisible(bool v) { visible_ = v; }

protected:
    HWND hwnd_ = nullptr;
    bool visible_ = false;
};

namespace dockutil {
// Subclass `container` so that any WM_NOTIFY it receives from its children is
// forwarded to its parent window. Used by panels whose visible content is a
// TreeView/ListView hosted inside a static container — WM_NOTIFY does not
// bubble on its own.
void ForwardNotifyToParent(HWND container);
} // namespace dockutil

} // namespace npp
