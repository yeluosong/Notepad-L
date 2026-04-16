# Build Lexilla as a static library (all stock lexers + LexerModule registry).

file(GLOB LEXILLA_SRC_LEXLIB "${LEXILLA_DIR}/lexlib/*.cxx")
file(GLOB LEXILLA_SRC_LEXERS "${LEXILLA_DIR}/lexers/*.cxx")
file(GLOB LEXILLA_SRC_ACCESS "${LEXILLA_DIR}/src/*.cxx")

add_library(lexilla STATIC
    ${LEXILLA_SRC_LEXLIB}
    ${LEXILLA_SRC_LEXERS}
    ${LEXILLA_SRC_ACCESS})

target_include_directories(lexilla PUBLIC
    "${LEXILLA_DIR}/include"
    "${LEXILLA_DIR}/lexlib"
    "${LEXILLA_DIR}/src"
    "${SCINTILLA_DIR}/include")

target_compile_definitions(lexilla PRIVATE STATIC_BUILD)

if(MSVC)
    target_compile_options(lexilla PRIVATE /wd4244 /wd4267 /wd4456 /wd4458)
endif()
