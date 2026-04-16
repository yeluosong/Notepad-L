#include "FindReplaceDlg.h"
#include "ScintillaEditView.h"
#include "../MISC/Common/StringUtil.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>

#include <commctrl.h>
#include <windowsx.h>
#include <string>
#include <vector>

namespace npp {

namespace {

constexpr int kDlgW = 440;
constexpr int kDlgH = 220;

// Runtime-built dialog template so we don't need a separate .rc entry.
struct DlgTemplateBuilder
{
    std::vector<BYTE> bytes;

    void Align(size_t n) {
        while (bytes.size() % n) bytes.push_back(0);
    }
    template<typename T> void Push(const T& v) {
        const BYTE* p = reinterpret_cast<const BYTE*>(&v);
        bytes.insert(bytes.end(), p, p + sizeof(T));
    }
    void PushWStr(const wchar_t* s) {
        while (*s) { Push<wchar_t>(*s++); }
        Push<wchar_t>(0);
    }
    void AddHeader(DWORD style, short x, short y, short cx, short cy,
                   const wchar_t* title, const wchar_t* font, short fontSize) {
        DLGTEMPLATE dt{};
        dt.style = style | DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS;
        dt.dwExtendedStyle = 0;
        dt.cdit = 0;
        dt.x = x; dt.y = y; dt.cx = cx; dt.cy = cy;
        Push(dt);
        Push<WORD>(0); // menu
        Push<WORD>(0); // class
        PushWStr(title);
        Push<WORD>(fontSize);
        PushWStr(font);
    }
    void AddItem(DWORD style, DWORD exStyle, short x, short y, short cx, short cy,
                 WORD id, WORD ctrlClassAtom, const wchar_t* text)
    {
        Align(sizeof(DWORD));
        DLGITEMTEMPLATE it{};
        it.style = style | WS_CHILD | WS_VISIBLE;
        it.dwExtendedStyle = exStyle;
        it.x = x; it.y = y; it.cx = cx; it.cy = cy;
        it.id = id;
        Push(it);
        Push<WORD>(0xFFFF);
        Push<WORD>(ctrlClassAtom);
        PushWStr(text);
        Push<WORD>(0); // no creation data
    }
    void IncrementItemCount(size_t count) {
        reinterpret_cast<DLGTEMPLATE*>(bytes.data())->cdit =
            static_cast<WORD>(count);
    }
};

enum : WORD {
    kClassBtn  = 0x0080,
    kClassEdit = 0x0081,
    kClassStatic = 0x0082,
};

} // namespace

INT_PTR CALLBACK FindReplaceDlg::DlgProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    FindReplaceDlg* self = nullptr;
    if (m == WM_INITDIALOG) {
        self = reinterpret_cast<FindReplaceDlg*>(l);
        ::SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    } else {
        self = reinterpret_cast<FindReplaceDlg*>(
            ::GetWindowLongPtrW(h, GWLP_USERDATA));
    }
    return self ? self->HandleMessage(h, m, w, l) : FALSE;
}

// Anchor the dialog to the top-right corner of the active editor's window.
static void AnchorTopRight(HWND dlg, HWND anchorTo)
{
    if (!dlg || !anchorTo) return;
    RECT er{}, dr{};
    ::GetWindowRect(anchorTo, &er);
    ::GetWindowRect(dlg, &dr);
    const int dw = dr.right - dr.left;
    const int dh = dr.bottom - dr.top;
    constexpr int kMargin = 8;
    int x = er.right - dw - kMargin;
    int y = er.top   + kMargin;
    if (x < er.left + kMargin) x = er.left + kMargin;
    ::SetWindowPos(dlg, HWND_TOP, x, y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void FindReplaceDlg::Show(HWND owner, HINSTANCE hInst, ScintillaEditView* view, FindMode mode)
{
    view_ = view;
    mode_ = mode;
    if (hwnd_) {
        SetControlsForMode(hwnd_, mode);
        AnchorTopRight(hwnd_, view_ ? view_->Hwnd() : owner);
        ::ShowWindow(hwnd_, SW_SHOW);
        ::SetForegroundWindow(hwnd_);
        return;
    }

    // Vertical, sectioned layout modeled after the Visual-Studio
    // "Find and Replace" tool window: two mode-toggle buttons at top,
    // a single Find what / (optional) Replace with column, a "Look in"
    // combo, a grouped "Find options" panel, then a row of action
    // buttons at the bottom.
    // Built with the Replace-mode (taller) layout. In Find mode the
    // Replace row is hidden and the controls below it slide up.
    constexpr short kW = 244, kH = 218;
    DlgTemplateBuilder b;
    b.AddHeader(WS_POPUP | WS_CAPTION | WS_SYSMENU,
        0, 0, kW, kH,
        L"Find and Replace", L"Segoe UI", 9);

    auto AddLabel = [&](short x, short y, short cx, WORD id, const wchar_t* t){
        b.AddItem(SS_LEFT, 0, x, y, cx, 9, id, kClassStatic, t);
    };
    auto AddEdit = [&](short x, short y, short cx, WORD id){
        b.AddItem(WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE,
            x, y, cx, 14, id, kClassEdit, L"");
    };
    auto AddCheck = [&](short x, short y, short cx, WORD id, const wchar_t* t){
        b.AddItem(BS_AUTOCHECKBOX | WS_TABSTOP, 0,
            x, y, cx, 10, id, kClassBtn, t);
    };
    auto AddGroup = [&](short x, short y, short cx, short cy, WORD id, const wchar_t* t){
        b.AddItem(BS_GROUPBOX, 0, x, y, cx, cy, id, kClassBtn, t);
    };
    auto AddBtn = [&](short x, short y, short cx, WORD id, const wchar_t* t, bool def = false){
        DWORD st = (def ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP;
        b.AddItem(st, 0, x, y, cx, 16, id, kClassBtn, t);
    };
    auto AddCombo = [&](short x, short y, short cx, WORD id){
        b.AddItem(CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL,
            0, x, y, cx, 90, id, 0x0085 /*COMBOBOX*/, L"");
    };

    constexpr short kPad = 10;
    constexpr short kInner = kW - 2 * kPad;   // 224

    // --- Mode-toggle row at top ---
    AddBtn(kPad,         4, 100, IDC_QUICK_FIND,    L"Quick &Find",    true);
    AddBtn(kPad + 108,   4, 100, IDC_QUICK_REPLACE, L"Quick Re&place");

    // --- Find what ---
    AddLabel(kPad, 26, kInner, 0xFFFF, L"Find what:");
    AddEdit (kPad, 36, kInner, IDC_FIND_WHAT);

    // --- Replace with (visible only in Replace mode) ---
    AddLabel(kPad, 54, kInner, IDC_REPLACE_LABEL, L"Replace with:");
    AddEdit (kPad, 64, kInner, IDC_REPLACE_WITH);

    // --- Look in combo ---
    AddLabel(kPad, 84, kInner, IDC_LOOKIN_LABEL, L"Look in:");
    AddCombo(kPad, 94, kInner, IDC_LOOKIN);

    // --- Find options group (tight 12-dlu line spacing) ---
    AddGroup(kPad,     114, kInner, 76, IDC_OPTIONS_GROUP, L"Find options");
    AddCheck(kPad + 8, 126, kInner - 16, IDC_CASE,      L"Match &case");
    AddCheck(kPad + 8, 138, kInner - 16, IDC_WHOLEWORD, L"Match &whole word");
    AddCheck(kPad + 8, 150, kInner - 16, IDC_UPWARD,    L"Search &up");
    AddCheck(kPad + 8, 162, kInner - 16, IDC_WRAP,      L"Wra&p around");
    AddCheck(kPad + 8, 174, kInner - 16, IDC_REGEX,     L"Use regular e&xpression");

    // --- Status line (sits in the 4-dlu margin between group and buttons) ---
    AddLabel(kPad, 194, kInner, IDC_STATUS, L"");

    // --- Bottom action row ---
    constexpr short kBtnY = 200;
    AddBtn(kPad,         kBtnY, 56, IDC_FIND_NEXT,   L"Find &Next", true);
    AddBtn(kPad + 60,    kBtnY, 56, IDC_REPLACE_ONE, L"&Replace");
    AddBtn(kPad + 120,   kBtnY, 56, IDC_MARK_ALL,    L"&Bookmark All");
    AddBtn(kPad + 180,   kBtnY, 44, IDCANCEL,        L"Close");

    // 4 labels + 2 edits + 5 checks + 1 groupbox + 1 combo + 6 buttons = 19
    b.IncrementItemCount(19);

    hwnd_ = ::CreateDialogIndirectParamW(hInst,
        reinterpret_cast<LPCDLGTEMPLATE>(b.bytes.data()),
        owner, &FindReplaceDlg::DlgProc,
        reinterpret_cast<LPARAM>(this));
    if (hwnd_) {
        SetControlsForMode(hwnd_, mode);
        AnchorTopRight(hwnd_, view_ ? view_->Hwnd() : owner);
        ::ShowWindow(hwnd_, SW_SHOW);
    }
}

void FindReplaceDlg::Close()
{
    if (hwnd_) { ::DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void FindReplaceDlg::SetControlsForMode(HWND h, FindMode mode)
{
    ::SetWindowTextW(h, L"Find and Replace");

    const bool repl = (mode == FindMode::Replace);
    const int show = repl ? SW_SHOW : SW_HIDE;
    ::ShowWindow(::GetDlgItem(h, IDC_REPLACE_LABEL), show);
    ::ShowWindow(::GetDlgItem(h, IDC_REPLACE_WITH),  show);
    ::ShowWindow(::GetDlgItem(h, IDC_REPLACE_ONE),   SW_SHOW);

    // Dialog template is built in Replace layout; in Find mode we slide
    // every control below the (hidden) Replace row up by 30 dlu (label
    // 9 + edit 14 + 7 spacing) and shrink the dialog by the same amount.
    static constexpr int kShiftDlu = 30;
    RECT delta{0, 0, 0, kShiftDlu};
    ::MapDialogRect(h, &delta);
    const int shiftPx = delta.bottom;
    const bool wantShifted = !repl;
    if (wantShifted != shiftedUp_) {
        const int dy = wantShifted ? -shiftPx : shiftPx;
        const int ids[] = {
            IDC_LOOKIN_LABEL, IDC_LOOKIN, IDC_OPTIONS_GROUP,
            IDC_CASE, IDC_WHOLEWORD, IDC_UPWARD, IDC_WRAP, IDC_REGEX,
            IDC_STATUS, IDC_FIND_NEXT, IDC_REPLACE_ONE,
            IDC_MARK_ALL, IDCANCEL,
        };
        for (int id : ids) {
            HWND c = ::GetDlgItem(h, id);
            if (!c) continue;
            RECT r{}; ::GetWindowRect(c, &r);
            ::MapWindowPoints(nullptr, h,
                reinterpret_cast<POINT*>(&r), 2);
            ::SetWindowPos(c, nullptr, r.left, r.top + dy,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        // Shrink/grow the dialog itself by the same amount.
        RECT wr{}; ::GetWindowRect(h, &wr);
        ::SetWindowPos(h, nullptr, 0, 0,
            wr.right - wr.left, (wr.bottom - wr.top) + dy,
            SWP_NOMOVE | SWP_NOZORDER);
        shiftedUp_ = wantShifted;
    }

    // Seed Look-in combo with a single (decorative) option.
    HWND combo = ::GetDlgItem(h, IDC_LOOKIN);
    if (combo && ::SendMessageW(combo, CB_GETCOUNT, 0, 0) == 0) {
        ::SendMessageW(combo, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(L"Current Document"));
        ::SendMessageW(combo, CB_SETCURSEL, 0, 0);
    }

    ::CheckDlgButton(h, IDC_CASE,      optCase_  ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(h, IDC_WHOLEWORD, optWhole_ ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(h, IDC_REGEX,     optRegex_ ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(h, IDC_WRAP,      optWrap_  ? BST_CHECKED : BST_UNCHECKED);
    ::CheckDlgButton(h, IDC_UPWARD,    optUp_    ? BST_CHECKED : BST_UNCHECKED);

    UINT primary = repl ? IDC_REPLACE_ONE : IDC_FIND_NEXT;
    ::SendMessageW(h, DM_SETDEFID, primary, 0);
    ::SetFocus(::GetDlgItem(h, repl ? IDC_REPLACE_WITH : IDC_FIND_WHAT));
}

void FindReplaceDlg::ReadOptions(HWND h)
{
    optCase_  = ::IsDlgButtonChecked(h, IDC_CASE)      == BST_CHECKED;
    optWhole_ = ::IsDlgButtonChecked(h, IDC_WHOLEWORD) == BST_CHECKED;
    optRegex_ = ::IsDlgButtonChecked(h, IDC_REGEX)     == BST_CHECKED;
    optWrap_  = ::IsDlgButtonChecked(h, IDC_WRAP)      == BST_CHECKED;
    optUp_    = ::IsDlgButtonChecked(h, IDC_UPWARD)    == BST_CHECKED;
}

int FindReplaceDlg::BuildSearchFlags() const
{
    int f = 0;
    if (optCase_)  f |= SCFIND_MATCHCASE;
    if (optWhole_) f |= SCFIND_WHOLEWORD;
    if (optRegex_) f |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
    return f;
}

std::string FindReplaceDlg::GetFindUtf8(HWND h) const
{
    wchar_t buf[4096] = L"";
    ::GetDlgItemTextW(h, IDC_FIND_WHAT, buf, ARRAYSIZE(buf));
    return WideToUtf8(buf);
}

std::string FindReplaceDlg::GetReplaceUtf8(HWND h) const
{
    wchar_t buf[4096] = L"";
    ::GetDlgItemTextW(h, IDC_REPLACE_WITH, buf, ARRAYSIZE(buf));
    return WideToUtf8(buf);
}

void FindReplaceDlg::SetStatus(HWND h, const std::wstring& msg)
{
    ::SetDlgItemTextW(h, IDC_STATUS, msg.c_str());
}

bool FindReplaceDlg::DoFind(ScintillaEditView* view, bool forward, bool reportWrap)
{
    if (!view || lastFind_.empty()) return false;

    const sptr_t docEnd = view->Call(SCI_GETLENGTH);
    const sptr_t caret  = view->Call(SCI_GETCURRENTPOS);
    const sptr_t anchor = view->Call(SCI_GETANCHOR);
    sptr_t start = forward ? std::max(caret, anchor)
                           : std::min(caret, anchor);
    sptr_t end   = forward ? docEnd : 0;

    view->Call(SCI_SETSEARCHFLAGS, static_cast<uptr_t>(BuildSearchFlags()));
    view->Call(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
    sptr_t pos = view->Call(SCI_SEARCHINTARGET,
        static_cast<uptr_t>(lastFind_.size()),
        reinterpret_cast<sptr_t>(lastFind_.data()));

    if (pos < 0 && optWrap_) {
        start = forward ? 0 : docEnd;
        end   = forward ? docEnd : 0;
        view->Call(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
        pos = view->Call(SCI_SEARCHINTARGET,
            static_cast<uptr_t>(lastFind_.size()),
            reinterpret_cast<sptr_t>(lastFind_.data()));
        if (pos >= 0 && reportWrap && hwnd_)
            SetStatus(hwnd_, L"Search wrapped.");
    }
    if (pos < 0) {
        if (hwnd_) SetStatus(hwnd_, L"No match.");
        ::MessageBeep(MB_ICONINFORMATION);
        return false;
    }
    sptr_t matchEnd = view->Call(SCI_GETTARGETEND);
    view->Call(SCI_SETSEL, static_cast<uptr_t>(pos), matchEnd);
    view->Call(SCI_SCROLLCARET);
    return true;
}

int FindReplaceDlg::DoReplaceAll(ScintillaEditView* view)
{
    if (!view || lastFind_.empty()) return 0;

    view->Call(SCI_SETSEARCHFLAGS, static_cast<uptr_t>(BuildSearchFlags()));
    view->Call(SCI_BEGINUNDOACTION);
    sptr_t start = 0, end = view->Call(SCI_GETLENGTH);
    int replaced = 0;

    while (true) {
        view->Call(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
        sptr_t pos = view->Call(SCI_SEARCHINTARGET,
            static_cast<uptr_t>(lastFind_.size()),
            reinterpret_cast<sptr_t>(lastFind_.data()));
        if (pos < 0) break;
        sptr_t replaceLen = optRegex_
            ? view->Call(SCI_REPLACETARGETRE,
                  static_cast<uptr_t>(lastReplace_.size()),
                  reinterpret_cast<sptr_t>(lastReplace_.data()))
            : view->Call(SCI_REPLACETARGET,
                  static_cast<uptr_t>(lastReplace_.size()),
                  reinterpret_cast<sptr_t>(lastReplace_.data()));
        start = pos + replaceLen;
        end   = view->Call(SCI_GETLENGTH);
        ++replaced;
        if (replaceLen == 0) ++start; // empty match guard
    }
    view->Call(SCI_ENDUNDOACTION);
    return replaced;
}

int FindReplaceDlg::DoCount(ScintillaEditView* view)
{
    if (!view || lastFind_.empty()) return 0;
    view->Call(SCI_SETSEARCHFLAGS, static_cast<uptr_t>(BuildSearchFlags()));
    int count = 0;
    sptr_t start = 0, end = view->Call(SCI_GETLENGTH);
    while (true) {
        view->Call(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
        sptr_t pos = view->Call(SCI_SEARCHINTARGET,
            static_cast<uptr_t>(lastFind_.size()),
            reinterpret_cast<sptr_t>(lastFind_.data()));
        if (pos < 0) break;
        sptr_t tend = view->Call(SCI_GETTARGETEND);
        ++count;
        start = (tend == pos) ? pos + 1 : tend;
    }
    return count;
}

int FindReplaceDlg::DoMarkAll(ScintillaEditView* view)
{
    if (!view) return 0;
    constexpr int kIndic = 31;
    view->Call(SCI_INDICSETSTYLE, kIndic, INDIC_ROUNDBOX);
    view->Call(SCI_INDICSETFORE,  kIndic, 0x00FFFF); // yellow
    view->Call(SCI_INDICSETALPHA, kIndic, 90);
    view->Call(SCI_SETINDICATORCURRENT, kIndic);
    view->Call(SCI_INDICATORCLEARRANGE, 0,
        view->Call(SCI_GETLENGTH));

    if (lastFind_.empty()) return 0;
    view->Call(SCI_SETSEARCHFLAGS, static_cast<uptr_t>(BuildSearchFlags()));
    int count = 0;
    sptr_t start = 0, end = view->Call(SCI_GETLENGTH);
    while (true) {
        view->Call(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
        sptr_t pos = view->Call(SCI_SEARCHINTARGET,
            static_cast<uptr_t>(lastFind_.size()),
            reinterpret_cast<sptr_t>(lastFind_.data()));
        if (pos < 0) break;
        sptr_t tend = view->Call(SCI_GETTARGETEND);
        view->Call(SCI_INDICATORFILLRANGE,
            static_cast<uptr_t>(pos), tend - pos);
        ++count;
        start = (tend == pos) ? pos + 1 : tend;
    }
    return count;
}

bool FindReplaceDlg::FindNextAgain(ScintillaEditView* view)
{
    return DoFind(view, /*forward*/ !optUp_, /*reportWrap*/ false);
}

bool FindReplaceDlg::FindPrevAgain(ScintillaEditView* view)
{
    return DoFind(view, /*forward*/  optUp_, /*reportWrap*/ false);
}

INT_PTR FindReplaceDlg::HandleMessage(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_INITDIALOG: {
        if (!lastFind_.empty()) {
            ::SetDlgItemTextW(h, IDC_FIND_WHAT, Utf8ToWide(lastFind_).c_str());
        }
        if (!lastReplace_.empty()) {
            ::SetDlgItemTextW(h, IDC_REPLACE_WITH, Utf8ToWide(lastReplace_).c_str());
        }
        return TRUE;
    }
    case WM_COMMAND: {
        const WORD id = LOWORD(w);
        switch (id) {
        case IDCANCEL:
            Close();
            return TRUE;

        case IDC_QUICK_FIND:
            mode_ = FindMode::Find;
            SetControlsForMode(h, mode_);
            return TRUE;
        case IDC_QUICK_REPLACE:
            mode_ = FindMode::Replace;
            SetControlsForMode(h, mode_);
            return TRUE;

        case IDC_FIND_NEXT: {
            ReadOptions(h);
            lastFind_ = GetFindUtf8(h);
            DoFind(view_, /*forward*/ !optUp_, /*reportWrap*/ true);
            return TRUE;
        }
        case IDC_FIND_PREV: {
            ReadOptions(h);
            lastFind_ = GetFindUtf8(h);
            DoFind(view_, /*forward*/ optUp_, /*reportWrap*/ true);
            return TRUE;
        }
        case IDC_COUNT: {
            ReadOptions(h);
            lastFind_ = GetFindUtf8(h);
            int c = DoCount(view_);
            wchar_t buf[64];
            ::swprintf_s(buf, L"%d match(es).", c);
            SetStatus(h, buf);
            return TRUE;
        }
        case IDC_MARK_ALL: {
            ReadOptions(h);
            lastFind_ = GetFindUtf8(h);
            int c = DoMarkAll(view_);
            wchar_t buf[64];
            ::swprintf_s(buf, L"%d marked.", c);
            SetStatus(h, buf);
            return TRUE;
        }
        case IDC_REPLACE_ONE: {
            ReadOptions(h);
            lastFind_    = GetFindUtf8(h);
            lastReplace_ = GetReplaceUtf8(h);
            // If current selection == match, replace it; else just find.
            sptr_t selS = view_->Call(SCI_GETSELECTIONSTART);
            sptr_t selE = view_->Call(SCI_GETSELECTIONEND);
            if (selE > selS) {
                view_->Call(SCI_SETTARGETRANGE,
                    static_cast<uptr_t>(selS), selE);
                sptr_t pos = view_->Call(SCI_SEARCHINTARGET,
                    static_cast<uptr_t>(lastFind_.size()),
                    reinterpret_cast<sptr_t>(lastFind_.data()));
                if (pos == selS) {
                    if (optRegex_)
                        view_->Call(SCI_REPLACETARGETRE,
                            static_cast<uptr_t>(lastReplace_.size()),
                            reinterpret_cast<sptr_t>(lastReplace_.data()));
                    else
                        view_->Call(SCI_REPLACETARGET,
                            static_cast<uptr_t>(lastReplace_.size()),
                            reinterpret_cast<sptr_t>(lastReplace_.data()));
                }
            }
            DoFind(view_, /*forward*/ !optUp_, /*reportWrap*/ true);
            return TRUE;
        }
        case IDC_REPLACE_ALL: {
            ReadOptions(h);
            lastFind_    = GetFindUtf8(h);
            lastReplace_ = GetReplaceUtf8(h);
            int n = DoReplaceAll(view_);
            wchar_t buf[64];
            ::swprintf_s(buf, L"%d replacement(s).", n);
            SetStatus(h, buf);
            return TRUE;
        }
        }
        break;
    }
    case WM_CLOSE:
        Close();
        return TRUE;
    case WM_DESTROY:
        hwnd_ = nullptr;
        return TRUE;
    }
    return FALSE;
}

} // namespace npp
