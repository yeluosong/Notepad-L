#pragma once
#include <string>
#include <string_view>

namespace npp {

// Keep enum order stable; indices are persisted in session.xml (M7).
enum class LangType : int
{
    Text = 0,
    C, Cpp, CSharp, Java, ObjectiveC, Rust, Go, Swift,
    Python, Ruby, Perl, Lua, Tcl,
    JavaScript, TypeScript, Json, Yaml, Toml,
    Html, Xml, Css, Scss, Less,
    PhP,
    Shell, Batch, PowerShell,
    Sql, Ini, Makefile, CMake, Diff, Markdown,
    Kotlin, Dart, Scala, Haskell, Erlang, Elixir, Fsharp,
    VB, Pascal, Lisp, Scheme, Clojure, Fortran, Ada, R, Matlab,
    AsciiDoc, Tex, Props,
    Count_
};

const wchar_t* LangTypeName(LangType t);

// Maps a Scintilla Lexilla lexer id (e.g. "cpp", "python").
const char* LangLexerName(LangType t);

// Infer language from file extension (leading dot optional, case insensitive).
LangType LangFromExt(std::wstring_view ext);

// Infer language from file path — respects exact filenames (Makefile, CMakeLists.txt, ...).
LangType LangFromPath(const std::wstring& path);

} // namespace npp
