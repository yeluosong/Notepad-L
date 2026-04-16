#include "FunctionListPanel.h"
#include "../../ScintillaComponent/ScintillaEditView.h"
#include "../../MISC/Common/StringUtil.h"

#include <commctrl.h>
#include <regex>
#include <sstream>

namespace npp {

HWND FunctionListPanel::Create(HWND parent, HINSTANCE hInst)
{
    hwnd_ = ::CreateWindowExW(
        WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_CHILD | WS_CLIPCHILDREN,
        0, 0, 0, 0, parent, nullptr, hInst, nullptr);
    tree_ = ::CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT |
        TVS_SHOWSELALWAYS | TVS_FULLROWSELECT,
        0, 0, 10, 10, hwnd_, nullptr, hInst, nullptr);
    dockutil::ForwardNotifyToParent(hwnd_);
    return hwnd_;
}

void FunctionListPanel::Resize(const RECT& inner)
{
    ::SetWindowPos(tree_, nullptr, 0, 0,
        inner.right - inner.left, inner.bottom - inner.top,
        SWP_NOZORDER);
}

bool FunctionListPanel::ParseByRegex(const std::string& utf8, LangType lang,
                                      std::vector<Item>& out)
{
    out.clear();
    std::regex re;
    bool have = false;
    try {
        switch (lang) {
        case LangType::Cpp:
        case LangType::C:
        case LangType::Java:
        case LangType::CSharp:
            // crude: "return-type name(args) {" on one line (ignoring ctors/dtors).
            re = std::regex(R"(^[\w:<>\*\&,\s]{2,}?\s+([A-Za-z_][\w:]*)\s*\([^;{}]*\)\s*(?:const)?\s*\{?\s*$)",
                            std::regex::ECMAScript);
            have = true; break;
        case LangType::Python:
            re = std::regex(R"(^\s*(?:def|class)\s+([A-Za-z_]\w*))", std::regex::ECMAScript);
            have = true; break;
        case LangType::JavaScript:
        case LangType::TypeScript:
            re = std::regex(
                R"(^(?:\s*(?:export\s+)?(?:async\s+)?function\s+([A-Za-z_$][\w$]*)|\s*(?:export\s+)?class\s+([A-Za-z_$][\w$]*)|\s*(?:const|let|var)\s+([A-Za-z_$][\w$]*)\s*=\s*(?:async\s*)?(?:function|\([^)]*\)\s*=>)))",
                std::regex::ECMAScript);
            have = true; break;
        case LangType::Go:
            re = std::regex(R"(^\s*func\s+(?:\([^)]*\)\s*)?([A-Za-z_]\w*))", std::regex::ECMAScript);
            have = true; break;
        case LangType::Rust:
            re = std::regex(R"(^\s*(?:pub\s+)?(?:async\s+)?fn\s+([A-Za-z_]\w*))", std::regex::ECMAScript);
            have = true; break;
        case LangType::Ruby:
            re = std::regex(R"(^\s*(?:def|class|module)\s+([A-Za-z_]\w*))", std::regex::ECMAScript);
            have = true; break;
        case LangType::PhP:
            re = std::regex(R"(^\s*(?:public|private|protected|static|\s)*function\s+([A-Za-z_]\w*))", std::regex::ECMAScript);
            have = true; break;
        case LangType::Markdown:
            re = std::regex(R"(^(#+)\s+(.+?)\s*$)", std::regex::ECMAScript);
            have = true; break;
        default:
            return false;
        }
    } catch (...) { return false; }
    if (!have) return false;

    std::istringstream is(utf8);
    std::string line;
    int lineNum = 0;
    while (std::getline(is, line)) {
        ++lineNum;
        std::smatch m;
        if (std::regex_search(line, m, re)) {
            std::string hit;
            if (lang == LangType::Markdown) {
                hit = std::string(m[1].length(), '#') + " " + m[2].str();
            } else {
                for (size_t i = 1; i < m.size(); ++i) {
                    if (m[i].matched && m[i].length() > 0) { hit = m[i].str(); break; }
                }
            }
            if (!hit.empty()) {
                out.push_back({ Utf8ToWide(hit), lineNum });
            }
        }
    }
    return true;
}

void FunctionListPanel::Rebuild(ScintillaEditView& view, LangType lang)
{
    items_.clear();
    TreeView_DeleteAllItems(tree_);
    std::string txt = view.GetText();
    if (!ParseByRegex(txt, lang, items_)) return;

    for (const auto& it : items_) {
        TVINSERTSTRUCTW ti{};
        ti.hParent = TVI_ROOT;
        ti.hInsertAfter = TVI_LAST;
        ti.item.mask = TVIF_TEXT | TVIF_PARAM;
        wchar_t buf[512];
        _snwprintf_s(buf, 512, _TRUNCATE, L"%ls  : %d", it.label.c_str(), it.line);
        ti.item.pszText = buf;
        ti.item.lParam = it.line;
        TreeView_InsertItem(tree_, &ti);
    }
}

LRESULT FunctionListPanel::HandleNotify(LPARAM lParam)
{
    auto* nm = reinterpret_cast<LPNMHDR>(lParam);
    if (!nm || nm->hwndFrom != tree_) return 0;
    if (nm->code == NM_DBLCLK || nm->code == NM_RETURN) {
        HTREEITEM sel = TreeView_GetSelection(tree_);
        if (!sel) return 0;
        TVITEMW it{}; it.mask = TVIF_PARAM; it.hItem = sel;
        TreeView_GetItem(tree_, &it);
        if (onGoto_ && it.lParam > 0) onGoto_(static_cast<int>(it.lParam));
    }
    return 0;
}

} // namespace npp
