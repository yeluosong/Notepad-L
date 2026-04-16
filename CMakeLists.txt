cmake_minimum_required(VERSION 3.21)
project("NotePad-L"
    VERSION 0.1.0
    DESCRIPTION "NotePad-L (Win32 + Scintilla)"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "NotePad-L targets Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_compile_definitions(
    UNICODE _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    # Target Windows 7 as minimum supported OS
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    NTDDI_VERSION=0x06010000
)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc /MP /Zc:__cplusplus)
    add_link_options(/MANIFEST:NO /SUBSYSTEM:WINDOWS,6.01)
    # Static-link the CRT so Win7 doesn't need VCRUNTIME/MSVCP DLLs and avoids
    # the DLL-load-order crash in mtx_do_lock where CRITICAL_SECTION is NULL.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# --- Third-party: Scintilla + Lexilla (git submodules) ------------------------
set(SCINTILLA_DIR "${CMAKE_SOURCE_DIR}/scintilla"
    CACHE PATH "Path to Scintilla source tree")
set(LEXILLA_DIR   "${CMAKE_SOURCE_DIR}/lexilla"
    CACHE PATH "Path to Lexilla source tree")

if(NOT EXISTS "${SCINTILLA_DIR}/include/Scintilla.h")
    message(FATAL_ERROR
        "Scintilla not found at ${SCINTILLA_DIR}.\n"
        "Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS "${LEXILLA_DIR}/include/Lexilla.h")
    message(FATAL_ERROR
        "Lexilla not found at ${LEXILLA_DIR}.\n"
        "Run: git submodule update --init --recursive")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/Scintilla.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/Lexilla.cmake")

add_subdirectory(src)

# --- Installer (CPack + WiX) -------------------------------------------------
set(CPACK_PACKAGE_NAME               "NotePad-L")
set(CPACK_PACKAGE_VENDOR             "NotePad-L Project")
set(CPACK_PACKAGE_VERSION            "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "NotePad-L — a text editor built on Scintilla/Lexilla")
set(CPACK_PACKAGE_INSTALL_DIRECTORY  "NotePad-L")
set(CPACK_PACKAGE_FILE_NAME
    "NotePad-L-${PROJECT_VERSION}-win64")
set(CPACK_RESOURCE_FILE_LICENSE
    "${CMAKE_SOURCE_DIR}/resources/installer/License.rtf")

set(CPACK_GENERATOR "WIX")
# Stable upgrade GUID — regenerate only if the product identity changes.
set(CPACK_WIX_UPGRADE_GUID "6B9A3C2E-4F1D-4A1E-9C3A-1B0E9B2A7C01")
set(CPACK_WIX_PRODUCT_ICON
    "${CMAKE_SOURCE_DIR}/resources/icons/app.ico")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "NotePad-L")
set(CPACK_PACKAGE_EXECUTABLES "notepad-l" "NotePad-L")
set(CPACK_CREATE_DESKTOP_LINKS "notepad-l")
set(CPACK_WIX_ROOT "${CMAKE_SOURCE_DIR}/tools/wix")

include(CPack)
