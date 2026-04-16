#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct SCNotification;

namespace npp {

class ScintillaEditView;

struct MacroStep {
    int          message = 0;
    std::int64_t wParam  = 0;
    std::string  text;       // payload when lParam is a string (UTF-8)
    std::int64_t lParam  = 0;// numeric lParam when not a string
    bool         lParamIsText = false;
};

struct NamedMacro {
    std::wstring          name;
    std::vector<MacroStep> steps;
};

class MacroRecorder
{
public:
    void Start(ScintillaEditView* ed);
    void Stop(ScintillaEditView* ed);
    bool IsRecording() const { return recording_; }

    // Called from WM_NOTIFY when SCN_MACRORECORD fires on the active editor.
    void OnMacroRecord(const SCNotification& scn);

    // Replay current recording N times against the given view.
    void Play(ScintillaEditView* ed, int times);

    const std::vector<MacroStep>& Current() const { return current_; }
    bool HasCurrent() const { return !current_.empty(); }

    // Named library (in-memory; persistence via Parameters).
    std::vector<NamedMacro>& Library()             { return lib_; }
    const std::vector<NamedMacro>& Library() const { return lib_; }
    void SaveCurrentAs(const std::wstring& name);
    void PlayNamed(ScintillaEditView* ed, const std::wstring& name, int times);

private:
    void PlaySteps(ScintillaEditView* ed, const std::vector<MacroStep>& s, int times);

    std::vector<MacroStep>  current_;
    std::vector<NamedMacro> lib_;
    bool                    recording_ = false;
};

} // namespace npp
