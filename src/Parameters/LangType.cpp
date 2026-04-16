#include "LangType.h"
#include <algorithm>
#include <cwctype>
#include <string>

namespace npp {

namespace {

struct LangDef {
    LangType    id;
    const wchar_t* name;
    const char*    lexer;   // Lexilla CreateLexer name
};

// Kept in the same order as LangType for O(1) lookup.
const LangDef kLangs[] = {
    { LangType::Text,        L"Normal Text", ""         },
    { LangType::C,           L"C",           "cpp"      },
    { LangType::Cpp,         L"C++",         "cpp"      },
    { LangType::CSharp,      L"C#",          "cpp"      },
    { LangType::Java,        L"Java",        "cpp"      },
    { LangType::ObjectiveC,  L"Objective-C", "cpp"      },
    { LangType::Rust,        L"Rust",        "rust"     },
    { LangType::Go,          L"Go",          "cpp"      },
    { LangType::Swift,       L"Swift",       "cpp"      },
    { LangType::Python,      L"Python",      "python"   },
    { LangType::Ruby,        L"Ruby",        "ruby"     },
    { LangType::Perl,        L"Perl",        "perl"     },
    { LangType::Lua,         L"Lua",         "lua"      },
    { LangType::Tcl,         L"Tcl",         "tcl"      },
    { LangType::JavaScript,  L"JavaScript",  "cpp"      },
    { LangType::TypeScript,  L"TypeScript",  "cpp"      },
    { LangType::Json,        L"JSON",        "json"     },
    { LangType::Yaml,        L"YAML",        "yaml"     },
    { LangType::Toml,        L"TOML",        "toml"     },
    { LangType::Html,        L"HTML",        "hypertext"},
    { LangType::Xml,         L"XML",         "xml"      },
    { LangType::Css,         L"CSS",         "css"      },
    { LangType::Scss,        L"SCSS",        "css"      },
    { LangType::Less,        L"Less",        "css"      },
    { LangType::PhP,         L"PHP",         "hypertext"},
    { LangType::Shell,       L"Shell",       "bash"     },
    { LangType::Batch,       L"Batch",       "batch"    },
    { LangType::PowerShell,  L"PowerShell",  "powershell"},
    { LangType::Sql,         L"SQL",         "sql"      },
    { LangType::Ini,         L"INI",         "props"    },
    { LangType::Makefile,    L"Makefile",    "makefile" },
    { LangType::CMake,       L"CMake",       "cmake"    },
    { LangType::Diff,        L"Diff",        "diff"     },
    { LangType::Markdown,    L"Markdown",    "markdown" },
    { LangType::Kotlin,      L"Kotlin",      "cpp"      },
    { LangType::Dart,        L"Dart",        "cpp"      },
    { LangType::Scala,       L"Scala",       "cpp"      },
    { LangType::Haskell,     L"Haskell",     "haskell"  },
    { LangType::Erlang,      L"Erlang",      "erlang"   },
    { LangType::Elixir,      L"Elixir",      "erlang"   },
    { LangType::Fsharp,      L"F#",          "fsharp"   },
    { LangType::VB,          L"Visual Basic","vb"       },
    { LangType::Pascal,      L"Pascal",      "pascal"   },
    { LangType::Lisp,        L"Lisp",        "lisp"     },
    { LangType::Scheme,      L"Scheme",      "lisp"     },
    { LangType::Clojure,     L"Clojure",     "lisp"     },
    { LangType::Fortran,     L"Fortran",     "fortran"  },
    { LangType::Ada,         L"Ada",         "ada"      },
    { LangType::R,           L"R",           "r"        },
    { LangType::Matlab,      L"MATLAB",      "matlab"   },
    { LangType::AsciiDoc,    L"AsciiDoc",    "asciidoc" },
    { LangType::Tex,         L"TeX",         "tex"      },
    { LangType::Props,       L"Properties",  "props"    },
};
static_assert(sizeof(kLangs)/sizeof(kLangs[0]) == static_cast<size_t>(LangType::Count_),
              "kLangs table must match LangType enum");

struct ExtDef { const wchar_t* ext; LangType lang; };

// Extension -> language (order matters: first match wins).
const ExtDef kExtMap[] = {
    { L"c",    LangType::C }, { L"h",   LangType::C },
    { L"cpp",  LangType::Cpp }, { L"cxx", LangType::Cpp }, { L"cc", LangType::Cpp },
    { L"hpp",  LangType::Cpp }, { L"hxx", LangType::Cpp }, { L"hh", LangType::Cpp },
    { L"ipp",  LangType::Cpp }, { L"inl", LangType::Cpp },
    { L"cs",   LangType::CSharp },
    { L"java", LangType::Java },
    { L"m",    LangType::ObjectiveC }, { L"mm", LangType::ObjectiveC },
    { L"rs",   LangType::Rust },
    { L"go",   LangType::Go },
    { L"swift",LangType::Swift },
    { L"py",   LangType::Python }, { L"pyw", LangType::Python },
    { L"rb",   LangType::Ruby }, { L"rbw", LangType::Ruby },
    { L"pl",   LangType::Perl }, { L"pm",  LangType::Perl },
    { L"lua",  LangType::Lua },
    { L"tcl",  LangType::Tcl },
    { L"js",   LangType::JavaScript }, { L"mjs", LangType::JavaScript }, { L"cjs", LangType::JavaScript },
    { L"jsx",  LangType::JavaScript },
    { L"ts",   LangType::TypeScript }, { L"tsx", LangType::TypeScript },
    { L"json", LangType::Json }, { L"jsonc", LangType::Json },
    { L"yml",  LangType::Yaml }, { L"yaml", LangType::Yaml },
    { L"toml", LangType::Toml },
    { L"html", LangType::Html }, { L"htm", LangType::Html },
    { L"xml",  LangType::Xml }, { L"xsd", LangType::Xml }, { L"xsl", LangType::Xml },
    { L"xslt", LangType::Xml }, { L"svg", LangType::Xml }, { L"plist", LangType::Xml },
    { L"resx", LangType::Xml }, { L"csproj", LangType::Xml }, { L"vcxproj", LangType::Xml },
    { L"css",  LangType::Css },
    { L"scss", LangType::Scss }, { L"sass", LangType::Scss },
    { L"less", LangType::Less },
    { L"php",  LangType::PhP }, { L"phtml", LangType::PhP }, { L"php3", LangType::PhP },
    { L"sh",   LangType::Shell }, { L"bash", LangType::Shell }, { L"zsh", LangType::Shell },
    { L"bat",  LangType::Batch }, { L"cmd",  LangType::Batch },
    { L"ps1",  LangType::PowerShell }, { L"psm1", LangType::PowerShell },
    { L"sql",  LangType::Sql },
    { L"ini",  LangType::Ini }, { L"cfg", LangType::Ini }, { L"conf", LangType::Ini },
    { L"cmake",LangType::CMake },
    { L"diff", LangType::Diff }, { L"patch", LangType::Diff },
    { L"md",   LangType::Markdown }, { L"markdown", LangType::Markdown },
    { L"kt",   LangType::Kotlin }, { L"kts", LangType::Kotlin },
    { L"dart", LangType::Dart },
    { L"scala",LangType::Scala }, { L"sc", LangType::Scala },
    { L"hs",   LangType::Haskell },
    { L"erl",  LangType::Erlang }, { L"hrl", LangType::Erlang },
    { L"ex",   LangType::Elixir }, { L"exs", LangType::Elixir },
    { L"fs",   LangType::Fsharp }, { L"fsx", LangType::Fsharp },
    { L"vb",   LangType::VB }, { L"bas", LangType::VB },
    { L"pas",  LangType::Pascal }, { L"pp", LangType::Pascal },
    { L"lisp", LangType::Lisp }, { L"el", LangType::Lisp },
    { L"scm",  LangType::Scheme }, { L"ss", LangType::Scheme },
    { L"clj",  LangType::Clojure }, { L"cljs", LangType::Clojure }, { L"cljc", LangType::Clojure },
    { L"f",    LangType::Fortran }, { L"f90", LangType::Fortran }, { L"f95", LangType::Fortran },
    { L"for",  LangType::Fortran }, { L"f03", LangType::Fortran }, { L"f08", LangType::Fortran },
    { L"ada",  LangType::Ada }, { L"adb", LangType::Ada }, { L"ads", LangType::Ada },
    { L"r",    LangType::R }, { L"rmd", LangType::R },
    { L"matlab",LangType::Matlab },
    { L"adoc", LangType::AsciiDoc }, { L"asciidoc", LangType::AsciiDoc },
    { L"tex",  LangType::Tex }, { L"latex", LangType::Tex },
    { L"properties", LangType::Props },
    { L"txt",  LangType::Text }, { L"log", LangType::Text },
};

std::wstring ToLower(std::wstring_view s)
{
    std::wstring r(s);
    std::transform(r.begin(), r.end(), r.begin(),
        [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return r;
}

} // namespace

const wchar_t* LangTypeName(LangType t)
{
    const int i = static_cast<int>(t);
    if (i < 0 || i >= static_cast<int>(LangType::Count_)) return L"";
    return kLangs[i].name;
}

const char* LangLexerName(LangType t)
{
    const int i = static_cast<int>(t);
    if (i < 0 || i >= static_cast<int>(LangType::Count_)) return "";
    return kLangs[i].lexer;
}

LangType LangFromExt(std::wstring_view ext)
{
    std::wstring e = ToLower(ext);
    if (!e.empty() && e.front() == L'.') e.erase(e.begin());
    for (const auto& m : kExtMap) {
        if (e == m.ext) return m.lang;
    }
    return LangType::Text;
}

LangType LangFromPath(const std::wstring& path)
{
    // Extract file name.
    size_t slash = path.find_last_of(L"\\/");
    std::wstring name = (slash == std::wstring::npos) ? path : path.substr(slash + 1);
    std::wstring lower = ToLower(name);

    // Well-known filenames without extensions.
    if (lower == L"makefile" || lower == L"gnumakefile" || lower == L"makefile.am"
     || lower == L"makefile.in") return LangType::Makefile;
    if (lower == L"cmakelists.txt") return LangType::CMake;
    if (lower == L"dockerfile") return LangType::Shell;
    if (lower == L".gitignore" || lower == L".gitattributes") return LangType::Props;

    size_t dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos) return LangType::Text;
    return LangFromExt(name.substr(dot + 1));
}

} // namespace npp
