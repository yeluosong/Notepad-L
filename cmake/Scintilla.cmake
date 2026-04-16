# Build Scintilla as a static library (Win32 backend).
# Scintilla layout:
#   include/   public headers
#   src/       core
#   win32/     Win32 backend

file(GLOB SCI_SRC_CORE  "${SCINTILLA_DIR}/src/*.cxx")
file(GLOB SCI_SRC_WIN32 "${SCINTILLA_DIR}/win32/*.cxx")

# ScintillaDLL.cxx is the DLL entry and must not be linked into an .exe.
list(REMOVE_ITEM SCI_SRC_WIN32 "${SCINTILLA_DIR}/win32/ScintillaDLL.cxx")

add_library(scintilla STATIC ${SCI_SRC_CORE} ${SCI_SRC_WIN32})

target_include_directories(scintilla PUBLIC
    "${SCINTILLA_DIR}/include"
    "${SCINTILLA_DIR}/src"
    "${SCINTILLA_DIR}/win32")

target_compile_definitions(scintilla PRIVATE
    SCI_LEXER=0
    STATIC_BUILD
    _WIN32_WINNT=0x0601)

if(MSVC)
    target_compile_options(scintilla PRIVATE /wd4244 /wd4267 /wd4456 /wd4458)
endif()

target_link_libraries(scintilla PUBLIC imm32 msimg32 ole32 oleaut32 uuid)
