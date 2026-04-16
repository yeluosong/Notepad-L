#pragma once
#include <windows.h>

namespace npp {

class ScintillaEditView;

// Drives a side-by-side compare between two ScintillaEditView instances.
// Marks added/removed/changed lines with marker bitmaps and synchronizes
// vertical scroll while active.
class CompareController
{
public:
    bool IsActive() const { return active_; }

    // Compute diff between left/right buffers and apply line markers.
    // Both views must already display the buffers being compared.
    void Apply(ScintillaEditView& left, ScintillaEditView& right);

    // Remove markers and disable sync scroll.
    void Clear(ScintillaEditView& left, ScintillaEditView& right);

    // Called when a SCN_UPDATEUI fires on either view; mirrors first-visible
    // line across.
    void OnScroll(ScintillaEditView& src, ScintillaEditView& other);

private:
    bool active_      = false;
    bool inMirror_    = false;  // re-entry guard for OnScroll
};

} // namespace npp
