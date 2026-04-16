#include "MacroRecorder.h"
#include "../ScintillaComponent/ScintillaEditView.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <algorithm>

namespace npp {

namespace {

// Scintilla messages whose lParam is a const char* (text payload).
bool MessageHasTextLParam(int m)
{
    switch (m) {
    case SCI_REPLACESEL:
    case SCI_INSERTTEXT:
    case SCI_ADDTEXT:
    case SCI_APPENDTEXT:
    case SCI_SEARCHNEXT:
    case SCI_SEARCHPREV:
    case SCI_REPLACETARGET:
    case SCI_REPLACETARGETRE:
    case SCI_SEARCHINTARGET:
        return true;
    default:
        return false;
    }
}

} // namespace

void MacroRecorder::Start(ScintillaEditView* ed)
{
    if (!ed || recording_) return;
    current_.clear();
    ed->Call(SCI_STARTRECORD);
    recording_ = true;
}

void MacroRecorder::Stop(ScintillaEditView* ed)
{
    if (!ed || !recording_) return;
    ed->Call(SCI_STOPRECORD);
    recording_ = false;
}

void MacroRecorder::OnMacroRecord(const SCNotification& scn)
{
    if (!recording_) return;
    MacroStep s;
    s.message = scn.message;
    s.wParam  = static_cast<std::int64_t>(scn.wParam);
    if (MessageHasTextLParam(scn.message) && scn.lParam) {
        s.lParamIsText = true;
        s.text = reinterpret_cast<const char*>(scn.lParam);
    } else {
        s.lParam = static_cast<std::int64_t>(scn.lParam);
    }
    current_.push_back(std::move(s));
}

void MacroRecorder::PlaySteps(ScintillaEditView* ed,
                              const std::vector<MacroStep>& s, int times)
{
    if (!ed || s.empty() || times <= 0) return;
    ed->Call(SCI_BEGINUNDOACTION);
    for (int t = 0; t < times; ++t) {
        for (const auto& st : s) {
            sptr_t lp = st.lParamIsText
                ? reinterpret_cast<sptr_t>(st.text.c_str())
                : static_cast<sptr_t>(st.lParam);
            ed->Call(static_cast<unsigned int>(st.message),
                     static_cast<uptr_t>(st.wParam), lp);
        }
    }
    ed->Call(SCI_ENDUNDOACTION);
}

void MacroRecorder::Play(ScintillaEditView* ed, int times)
{
    PlaySteps(ed, current_, times);
}

void MacroRecorder::SaveCurrentAs(const std::wstring& name)
{
    if (current_.empty() || name.empty()) return;
    auto it = std::find_if(lib_.begin(), lib_.end(),
        [&](const NamedMacro& m){ return m.name == name; });
    if (it != lib_.end()) it->steps = current_;
    else lib_.push_back({ name, current_ });
}

void MacroRecorder::PlayNamed(ScintillaEditView* ed,
                              const std::wstring& name, int times)
{
    auto it = std::find_if(lib_.begin(), lib_.end(),
        [&](const NamedMacro& m){ return m.name == name; });
    if (it == lib_.end()) return;
    PlaySteps(ed, it->steps, times);
}

} // namespace npp
