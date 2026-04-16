#include "Stylers.h"

#include <Scintilla.h>
#include <ScintillaMessages.h>
#include <SciLexer.h>
#include <ILexer.h>
#include <Lexilla.h>

#include <cstring>

namespace npp {

namespace {

// Colours (Npp-ish "Default" stylers.xml palette).
constexpr COLORREF kFgDefault  = RGB(0x00,0x00,0x00);
constexpr COLORREF kBgDefault  = RGB(0xFF,0xFF,0xFF);
constexpr COLORREF kComment    = RGB(0x00,0x80,0x00);
constexpr COLORREF kLineComment= RGB(0x00,0x80,0x00);
constexpr COLORREF kNumber     = RGB(0xFF,0x80,0x00);
constexpr COLORREF kString     = RGB(0x80,0x80,0x80);
constexpr COLORREF kCharacter  = RGB(0x80,0x80,0x80);
constexpr COLORREF kKeyword    = RGB(0x00,0x00,0xFF);
constexpr COLORREF kKeyword2   = RGB(0x80,0x00,0x80);
constexpr COLORREF kPreproc    = RGB(0x80,0x40,0x20);
constexpr COLORREF kOperator   = RGB(0x00,0x00,0x80);
constexpr COLORREF kIdentifier = RGB(0x00,0x00,0x00);
constexpr COLORREF kTagName    = RGB(0x80,0x00,0x00);
constexpr COLORREF kAttrName   = RGB(0xFF,0x00,0x00);

void SetStyle(ScintillaEditView& v, int style, COLORREF fg, COLORREF bg = kBgDefault)
{
    v.Call(SCI_STYLESETFORE, static_cast<uptr_t>(style), static_cast<sptr_t>(fg));
    v.Call(SCI_STYLESETBACK, static_cast<uptr_t>(style), static_cast<sptr_t>(bg));
}

void ResetStyles(ScintillaEditView& v)
{
    v.Call(SCI_CLEARDOCUMENTSTYLE);
    v.Call(SCI_STYLESETFONT, STYLE_DEFAULT,
        reinterpret_cast<sptr_t>("Consolas"));
    v.Call(SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
    SetStyle(v, STYLE_DEFAULT, kFgDefault, kBgDefault);
    v.Call(SCI_STYLECLEARALL);
}

void ApplyKeywords(ScintillaEditView& v, int keywordSet, const char* words)
{
    v.Call(SCI_SETKEYWORDS, static_cast<uptr_t>(keywordSet),
        reinterpret_cast<sptr_t>(words));
}

// --- per-language style hook tables -----------------------------------------

void StyleCpp(ScintillaEditView& v)
{
    SetStyle(v, SCE_C_COMMENT,       kComment);
    SetStyle(v, SCE_C_COMMENTLINE,   kLineComment);
    SetStyle(v, SCE_C_COMMENTDOC,    kComment);
    SetStyle(v, SCE_C_COMMENTLINEDOC,kLineComment);
    SetStyle(v, SCE_C_NUMBER,        kNumber);
    SetStyle(v, SCE_C_WORD,          kKeyword);
    SetStyle(v, SCE_C_WORD2,         kKeyword2);
    SetStyle(v, SCE_C_STRING,        kString);
    SetStyle(v, SCE_C_CHARACTER,     kCharacter);
    SetStyle(v, SCE_C_OPERATOR,      kOperator);
    SetStyle(v, SCE_C_IDENTIFIER,    kIdentifier);
    SetStyle(v, SCE_C_PREPROCESSOR,  kPreproc);

    ApplyKeywords(v, 0,
        "alignas alignof and and_eq asm auto bitand bitor bool break case catch "
        "char char8_t char16_t char32_t class compl concept const consteval "
        "constexpr constinit const_cast continue co_await co_return co_yield "
        "decltype default delete do double dynamic_cast else enum explicit "
        "export extern false float for friend goto if inline int long mutable "
        "namespace new noexcept not not_eq nullptr operator or or_eq private "
        "protected public register reinterpret_cast requires return short "
        "signed sizeof static static_assert static_cast struct switch template "
        "this thread_local throw true try typedef typeid typename union "
        "unsigned using virtual void volatile wchar_t while xor xor_eq");
}

void StylePython(ScintillaEditView& v)
{
    SetStyle(v, SCE_P_COMMENTLINE,  kLineComment);
    SetStyle(v, SCE_P_COMMENTBLOCK, kComment);
    SetStyle(v, SCE_P_NUMBER,       kNumber);
    SetStyle(v, SCE_P_WORD,         kKeyword);
    SetStyle(v, SCE_P_STRING,       kString);
    SetStyle(v, SCE_P_CHARACTER,    kCharacter);
    SetStyle(v, SCE_P_OPERATOR,     kOperator);
    SetStyle(v, SCE_P_IDENTIFIER,   kIdentifier);
    SetStyle(v, SCE_P_TRIPLE,       kString);
    SetStyle(v, SCE_P_TRIPLEDOUBLE, kString);
    SetStyle(v, SCE_P_DEFNAME,      kKeyword2);
    SetStyle(v, SCE_P_CLASSNAME,    kKeyword2);

    ApplyKeywords(v, 0,
        "False None True and as assert async await break class continue def del "
        "elif else except finally for from global if import in is lambda "
        "nonlocal not or pass raise return try while with yield match case");
}

void StyleHtml(ScintillaEditView& v)
{
    SetStyle(v, SCE_H_TAG,          kTagName);
    SetStyle(v, SCE_H_TAGUNKNOWN,   kTagName);
    SetStyle(v, SCE_H_ATTRIBUTE,    kAttrName);
    SetStyle(v, SCE_H_DOUBLESTRING, kString);
    SetStyle(v, SCE_H_SINGLESTRING, kString);
    SetStyle(v, SCE_H_COMMENT,      kComment);
    SetStyle(v, SCE_H_NUMBER,       kNumber);
    SetStyle(v, SCE_H_ENTITY,       kKeyword2);
    SetStyle(v, SCE_H_TAGEND,       kOperator);
    SetStyle(v, SCE_H_XMLSTART,     kTagName);
    SetStyle(v, SCE_H_XMLEND,       kTagName);
}

void StyleCss(ScintillaEditView& v)
{
    SetStyle(v, SCE_CSS_COMMENT,          kComment);
    SetStyle(v, SCE_CSS_TAG,              kTagName);
    SetStyle(v, SCE_CSS_CLASS,            kKeyword2);
    SetStyle(v, SCE_CSS_PSEUDOCLASS,      kKeyword2);
    SetStyle(v, SCE_CSS_IDENTIFIER,       kKeyword);
    SetStyle(v, SCE_CSS_DOUBLESTRING,     kString);
    SetStyle(v, SCE_CSS_SINGLESTRING,     kString);
    SetStyle(v, SCE_CSS_OPERATOR,         kOperator);
    SetStyle(v, SCE_CSS_VALUE,            kIdentifier);
    SetStyle(v, SCE_CSS_IMPORTANT,        kKeyword);
}

void StyleJson(ScintillaEditView& v)
{
    SetStyle(v, SCE_JSON_NUMBER,          kNumber);
    SetStyle(v, SCE_JSON_STRING,          kString);
    SetStyle(v, SCE_JSON_PROPERTYNAME,    kKeyword2);
    SetStyle(v, SCE_JSON_KEYWORD,         kKeyword);
    SetStyle(v, SCE_JSON_LINECOMMENT,     kLineComment);
    SetStyle(v, SCE_JSON_BLOCKCOMMENT,    kComment);
    SetStyle(v, SCE_JSON_OPERATOR,        kOperator);
}

void StyleXml(ScintillaEditView& v)
{
    StyleHtml(v);
}

void StyleBash(ScintillaEditView& v)
{
    SetStyle(v, SCE_SH_COMMENTLINE, kLineComment);
    SetStyle(v, SCE_SH_NUMBER,      kNumber);
    SetStyle(v, SCE_SH_WORD,        kKeyword);
    SetStyle(v, SCE_SH_STRING,      kString);
    SetStyle(v, SCE_SH_CHARACTER,   kCharacter);
    SetStyle(v, SCE_SH_OPERATOR,    kOperator);
    SetStyle(v, SCE_SH_IDENTIFIER,  kIdentifier);
    SetStyle(v, SCE_SH_SCALAR,      kKeyword2);

    ApplyKeywords(v, 0,
        "if then else elif fi case esac for while until do done in function "
        "return break continue exit local readonly export declare typeset "
        "select time eval exec set unset shift trap source");
}

void StyleMarkdown(ScintillaEditView& v)
{
    // Headers H1..H6: warm orange-to-brown gradient, bold.
    constexpr int hStyles[6] = {
        SCE_MARKDOWN_HEADER1, SCE_MARKDOWN_HEADER2, SCE_MARKDOWN_HEADER3,
        SCE_MARKDOWN_HEADER4, SCE_MARKDOWN_HEADER5, SCE_MARKDOWN_HEADER6,
    };
    constexpr COLORREF hColors[6] = {
        RGB(0xC9,0x4A,0x1A), RGB(0xC9,0x6B,0x2C), RGB(0xB5,0x6E,0x2E),
        RGB(0x8E,0x5E,0x2E), RGB(0x6E,0x4E,0x2E), RGB(0x55,0x40,0x28),
    };
    for (int i = 0; i < 6; ++i) {
        SetStyle(v, hStyles[i], hColors[i]);
        v.Call(SCI_STYLESETBOLD, hStyles[i], 1);
        v.Call(SCI_STYLESETSIZE, hStyles[i], 14 - i); // 14,13,12,11,10,9
    }

    // Emphasis.
    SetStyle(v, SCE_MARKDOWN_EM1,           RGB(0x20,0x20,0x20));
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM1, 1);
    SetStyle(v, SCE_MARKDOWN_EM2,           RGB(0x20,0x20,0x20));
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_EM2, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG1,       RGB(0x20,0x20,0x20));
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG1, 1);
    SetStyle(v, SCE_MARKDOWN_STRONG2,       RGB(0x20,0x20,0x20));
    v.Call(SCI_STYLESETBOLD,   SCE_MARKDOWN_STRONG2, 1);

    // Code (inline + block) on cream background.
    constexpr COLORREF kCodeBg = RGB(0xF6,0xEE,0xDC);
    SetStyle(v, SCE_MARKDOWN_CODE,          RGB(0x80,0x40,0x20), kCodeBg);
    SetStyle(v, SCE_MARKDOWN_CODE2,         RGB(0x80,0x40,0x20), kCodeBg);
    SetStyle(v, SCE_MARKDOWN_CODEBK,        RGB(0x80,0x40,0x20), kCodeBg);

    // Lists, blockquote, rules, links.
    SetStyle(v, SCE_MARKDOWN_PRECHAR,       kKeyword2);
    SetStyle(v, SCE_MARKDOWN_ULIST_ITEM,    kKeyword);
    SetStyle(v, SCE_MARKDOWN_OLIST_ITEM,    kKeyword);
    SetStyle(v, SCE_MARKDOWN_BLOCKQUOTE,    RGB(0x60,0x60,0x60));
    v.Call(SCI_STYLESETITALIC, SCE_MARKDOWN_BLOCKQUOTE, 1);
    SetStyle(v, SCE_MARKDOWN_STRIKEOUT,     RGB(0x80,0x80,0x80));
    SetStyle(v, SCE_MARKDOWN_HRULE,         kKeyword2);
    SetStyle(v, SCE_MARKDOWN_LINK,          RGB(0x00,0x66,0xCC));
    v.Call(SCI_STYLESETUNDERLINE, SCE_MARKDOWN_LINK, 1);
}

void StyleGeneric(ScintillaEditView& /*v*/) { /* no-op */ }

// ---------------------------------------------------------------------------

void SetLexerByName(ScintillaEditView& v, const char* lexerName)
{
    if (!lexerName || !*lexerName) {
        v.Call(SCI_SETILEXER, 0, 0);
        return;
    }
    void* lexer = reinterpret_cast<void*>(CreateLexer(lexerName));
    v.Call(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
}

} // namespace

void ApplyLanguage(ScintillaEditView& v, LangType lang)
{
    ResetStyles(v);
    SetLexerByName(v, LangLexerName(lang));

    switch (lang) {
    case LangType::C:
    case LangType::Cpp:
    case LangType::CSharp:
    case LangType::Java:
    case LangType::ObjectiveC:
    case LangType::JavaScript:
    case LangType::TypeScript:
    case LangType::Go:
    case LangType::Swift:
    case LangType::Rust:
    case LangType::Kotlin:
    case LangType::Dart:
    case LangType::Scala:
        StyleCpp(v); break;
    case LangType::Python:
        StylePython(v); break;
    case LangType::Html:
    case LangType::PhP:
        StyleHtml(v); break;
    case LangType::Xml:
        StyleXml(v); break;
    case LangType::Css:
    case LangType::Scss:
    case LangType::Less:
        StyleCss(v); break;
    case LangType::Json:
        StyleJson(v); break;
    case LangType::Shell:
        StyleBash(v); break;
    case LangType::Markdown:
        StyleMarkdown(v); break;
    default:
        StyleGeneric(v); break;
    }

    v.Call(SCI_COLOURISE, 0, -1);
}

} // namespace npp
