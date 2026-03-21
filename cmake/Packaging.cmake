# cmake/Packaging.cmake
# CPack configuration for Compendia.
# Included at the end of CMakeLists.txt via: include(cmake/Packaging.cmake)
# Run packaging from the build directory with: cpack -G ZIP  (or NSIS, TGZ, etc.)

# ── Common metadata ────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME                "Compendia")
set(CPACK_PACKAGE_VENDOR              "Merrill Aldrich")
set(CPACK_PACKAGE_VERSION             "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Media asset manager for photographers, hobbyists, and archivists")
set(CPACK_PACKAGE_INSTALL_DIRECTORY   "Compendia")
set(CPACK_PACKAGE_EXECUTABLES         "compendia;Compendia")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

# ── Windows ────────────────────────────────────────────────────────────────────
if(WIN32)
    # Phase 1: ZIP archive.  NSIS will be added in Phase 2.
    set(CPACK_GENERATOR           "ZIP")
    set(CPACK_PACKAGE_FILE_NAME   "Compendia-${PROJECT_VERSION}-Windows-x64")

    # Future NSIS variables (uncomment when adding Phase 2):
    # set(CPACK_NSIS_DISPLAY_NAME          "Compendia ${PROJECT_VERSION}")
    # set(CPACK_NSIS_INSTALL_ROOT          "$PROGRAMFILES64")
    # set(CPACK_NSIS_MUI_ICON              "${CMAKE_CURRENT_SOURCE_DIR}/resources/compendia.ico")
    # set(CPACK_NSIS_MUI_UNIICON           "${CMAKE_CURRENT_SOURCE_DIR}/resources/compendia.ico")
    # set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    # set(CPACK_NSIS_URL_INFO_ABOUT        "https://github.com/merrillaldrich/compendia")
endif()

# ── Linux ──────────────────────────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    # Phase 1: TGZ archive.  AppImage / DEB will be added in Phase 3.
    set(CPACK_GENERATOR         "TGZ")
    set(CPACK_PACKAGE_FILE_NAME "Compendia-${PROJECT_VERSION}-Linux-x86_64")

    # Future DEB variables (uncomment when adding Phase 3):
    # set(CPACK_DEBIAN_PACKAGE_DEPENDS   "libqt6widgets6, libqt6multimedia6, libexif12")
    # set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Merrill Aldrich")
    # set(CPACK_DEBIAN_PACKAGE_SECTION    "graphics")
endif()

# ── macOS ──────────────────────────────────────────────────────────────────────
if(APPLE)
    # Phase 4: DMG.
    set(CPACK_GENERATOR         "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "Compendia-${PROJECT_VERSION}-macOS")
endif()

# Must be the last line — CPack snapshots all CPACK_* variables here.
include(CPack)
