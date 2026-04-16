#include "CompareController.h"
#include "LineDiff.h"
#include "../ScintillaComponent/ScintillaEditView.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>

namespace npp {

namespace {
constexpr int kMarkAdded   = 20;
constexpr int kMarkRemoved = 21;
constexpr int kMarkChanged = 22;

void SetupMarkers(ScintillaEditView& v)
{
    v.Call(SCI_MARKERDEFINE, kMarkAdded,   SC_MARK_BACKGROUND);
    v.Call(SCI_MARKERSETBACK, kMarkAdded,  0xC8FFC8);  // light green
    v.Call(SCI_MARKERDEFINE, kMarkRemoved, SC_MARK_BACKGROUND);
    v.Call(SCI_MARKERSETBACK, kMarkRemoved,0xC8C8FF);  // light red (BGR)
    v.Call(SCI_MARKERDEFINE, kMarkChanged, SC_MARK_BACKGROUND);
    v.Call(SCI_MARKERSETBACK, kMarkChanged,0xC8FFFF);  // light yellow
}

std::string ReadAllUtf8(ScintillaEditView& v)
{
    return v.GetText();
}

}  // namespace

void CompareController::Apply(ScintillaEditView& left, ScintillaEditView& right)
{
    SetupMarkers(left);
    SetupMarkers(right);

    auto a = SplitLines(ReadAllUtf8(left));
    auto b = SplitLines(ReadAllUtf8(right));
    auto diff = ComputeLineDiff(a, b);

    left.Call(SCI_MARKERDELETEALL, kMarkAdded);
    left.Call(SCI_MARKERDELETEALL, kMarkRemoved);
    left.Call(SCI_MARKERDELETEALL, kMarkChanged);
    right.Call(SCI_MARKERDELETEALL, kMarkAdded);
    right.Call(SCI_MARKERDELETEALL, kMarkRemoved);
    right.Call(SCI_MARKERDELETEALL, kMarkChanged);

    for (const auto& e : diff) {
        switch (e.op) {
        case DiffOp::Equal: break;
        case DiffOp::Add:
            right.Call(SCI_MARKERADD, static_cast<uptr_t>(e.b), kMarkAdded);
            break;
        case DiffOp::Remove:
            left.Call(SCI_MARKERADD,  static_cast<uptr_t>(e.a), kMarkRemoved);
            break;
        case DiffOp::Change:
            left.Call(SCI_MARKERADD,  static_cast<uptr_t>(e.a), kMarkChanged);
            right.Call(SCI_MARKERADD, static_cast<uptr_t>(e.b), kMarkChanged);
            break;
        }
    }
    active_ = true;
}

void CompareController::Clear(ScintillaEditView& left, ScintillaEditView& right)
{
    for (auto* v : { &left, &right }) {
        v->Call(SCI_MARKERDELETEALL, kMarkAdded);
        v->Call(SCI_MARKERDELETEALL, kMarkRemoved);
        v->Call(SCI_MARKERDELETEALL, kMarkChanged);
    }
    active_ = false;
}

void CompareController::OnScroll(ScintillaEditView& src, ScintillaEditView& other)
{
    if (!active_ || inMirror_) return;
    int first = static_cast<int>(src.Call(SCI_GETFIRSTVISIBLELINE));
    inMirror_ = true;
    other.Call(SCI_SETFIRSTVISIBLELINE, static_cast<uptr_t>(first));
    inMirror_ = false;
}

} // namespace npp
