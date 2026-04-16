cmake_minimum_required(VERSION 3.21)
project("Notepad++"
    VERSION 0.1.0
    DESCRIPTION "Notepad++ (Win32 + Scintilla)"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Notepad++ targets Windows only.")
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
set(CPACK_PACKAGE_NAME               "Notepad++")
set(CPACK_PACKAGE_VENDOR             "Notepad++ Project")
set(CPACK_PACKAGE_VERSION            "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Notepad++ — a text editor built on Scintilla/Lexilla")
set(CPACK_PACKAGE_INSTALL_DIRECTORY  "Notepad++")
set(CPACK_PACKAGE_FILE_NAME
    "Notepad++-${PROJECT_VERSION}-win64")
set(CPACK_RESOURCE_FILE_LICENSE
    "${CMAKE_SOURCE_DIR}/resources/installer/License.rtf")

set(CPACK_GENERATOR "WIX")
# Stable upgrade GUID — regenerate only if the product identity changes.
set(CPACK_WIX_UPGRADE_GUID "6B9A3C2E-4F1D-4A1E-9C3A-1B0E9B2A7C01")
set(CPACK_WIX_PRODUCT_ICON
    "${CMAKE_SOURCE_DIR}/resources/icons/app.ico")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "Notepad++")
set(CPACK_PACKAGE_EXECUTABLES "notepad++" "Notepad++")
set(CPACK_CREATE_DESKTOP_LINKS "notepad++")
set(CPACK_WIX_ROOT "${CMAKE_SOURCE_DIR}/tools/wix")

include(CPack)
