#pragma once
#include "DockPanel.h"
#include "../../Parameters/LangType.h"
#include <windows.h>
#include <string>
#include <vector>
#include <functional>

namespace npp {

class ScintillaEditView;

// Right dock. TreeView showing function/class/heading items for the current
// buffer. Items parsed with per-language regexes.
class FunctionListPanel : public DockPanel
{
public:
    HWND Create(HWND parent, HINSTANCE hInst) override;
    const wchar_t* Title() const override { return L"Function List"; }
    void Resize(const RECT& inner) override;

    // Reparse the editor's full text with the given language.
    void Rebuild(ScintillaEditView& view, LangType lang);

    // Jump to the line of the currently selected item.
    using GotoCb = std::function<void(int line)>;
    void SetOnGoto(GotoCb cb) { onGoto_ = std::move(cb); }

    LRESULT HandleNotify(LPARAM lParam);

private:
    struct Item { std::wstring label; int line; };
    HWND tree_ = nullptr;
    std::vector<Item> items_;
    GotoCb onGoto_;

    static bool ParseByRegex(const std::string& utf8, LangType lang,
                             std::vector<Item>& out);
};

} // namespace npp
