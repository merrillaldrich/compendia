cmake_minimum_required(VERSION 3.16)

project(compendia VERSION 0.1.1 LANGUAGES CXX)

# Generate version.h from version.h.in, substituting project version variables.
configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS MultimediaWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Svg)

#find_package(libexif REQUIRED) # this doesn't work
#find_package(libheif)

if(WIN32)
    # libexif does not provide CMAKE config, so link straight to files
    set(LIBEXIF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/src")
    set(LIBEXIF_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/lib/exif.lib")

    # libheif does not provide CMAKE config, so link straight to files
    set(LIBHEIF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/src")
    set(LIBHEIF_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/lib/heif.lib")
else()
    # Linux: use system-installed libraries.
    # Run: sudo apt install libexif-dev
    # libheif is expected at /usr/local (built from source).
    set(LIBEXIF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/src")
    find_library(LIBEXIF_LIBRARY NAMES exif REQUIRED)

    # Use system libheif (1.17.6) which has libde265/aom decoder plugins configured.
    # The custom-built /usr/local/lib/libheif.so.1 lacks decoder plugins.
    set(LIBHEIF_INCLUDE_DIR "/usr/local/include")
    set(LIBHEIF_LIBRARY "/usr/lib/x86_64-linux-gnu/libheif.so.1")
endif()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(compendia
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        tagset.cpp
        tag.cpp tagfamily.cpp taggedfile.cpp
        compendiacore.h tagfamily.h taggedfile.h  tagset.h
        compendiacore.cpp
        tag.h
        resources.qrc
        tagwidget.h tagwidget.cpp
        flowlayout.h flowlayout.cpp
        tagassignmentcontainer.h tagassignmentcontainer.cpp
        tagfamilywidget.h tagfamilywidget.cpp
        taggablelistview.h taggablelistview.cpp
        navlibrarycontainer.h navlibrarycontainer.cpp
        clickablelabel.h clickablelabel.cpp
        variablewidthlineedit.h variablewidthlineedit.cpp
        filterproxymodel.h filterproxymodel.cpp
        tagwidgetclosebutton.h tagwidgetclosebutton.cpp
        tagfamilywidgetcollapsebutton.h tagfamilywidgetcollapsebutton.cpp
        icongenerator.h icongenerator.cpp
        navfiltercontainer.h navfiltercontainer.cpp
        tagcontainer.h tagcontainer.cpp
        welcomehintcontainer.h welcomehintcontainer.cpp
        taggingwidget.h taggingwidget.cpp
        facerecognizer.h facerecognizer.cpp
        facerecognitionsettingsdialog.h facerecognitionsettingsdialog.cpp
        exifparser.h exifparser.cpp
        previewcontainer.h previewcontainer.cpp
        zoomablegraphicsview.h zoomablegraphicsview.cpp
        filenamedelegate.h filenamedelegate.cpp
        datefilterbutton.h datefilterbutton.cpp
        folderfilterlineedit.h folderfilterlineedit.cpp
        multiprogressbar.h multiprogressbar.cpp
        framegrabber.h framegrabber.cpp
        perceptualhasher.h perceptualhasher.cpp
        starratingwidget.h starratingwidget.cpp
        folderscanner.h folderscanner.cpp
        aboutdialog.h aboutdialog.cpp aboutdialog.ui

    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET compendia APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(compendia SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(compendia
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Add dlib library from 3rdParty subdirectory
add_subdirectory(3rdParty/src/dlib)
target_link_libraries(compendia PRIVATE dlib)

# facerecognizer.cpp instantiates dlib's DNN templates, which generate an object
# file with more than 65 535 COFF sections on Windows — exceeding the default
# 32-bit PE/COFF limit.  Enable the extended ("big") object format for that
# single translation unit to avoid the "file too big" assembler error.
if (MSVC)
    set_source_files_properties(facerecognizer.cpp PROPERTIES COMPILE_OPTIONS "/bigobj;/O2;/arch:AVX2")
else()
    # -O2      : aggressive inlining collapses per-instantiation functions,
    #            reducing the number of generated symbols
    # -g0      : disable debug-info emission for this file to speed up compilation
    # -mavx2   : enable 256-bit AVX2 so dlib's DNN convolutions use SIMD (~5-8x faster)
    # -mfma    : enable fused multiply-add instructions
    # Note: -Wa,-mbig-obj is MinGW/Windows-only; Linux ELF has no section count limit
    set_source_files_properties(facerecognizer.cpp PROPERTIES COMPILE_OPTIONS "-O2;-g0;-mavx2;-mfma")
endif()

# ---------------------------------------------------------------------------
# Face recognition model files (not part of the build — user must supply them)
# ---------------------------------------------------------------------------
# The cross-image face recognition feature (Find Faces menu action) requires two
# pre-trained dlib model files downloaded separately from http://dlib.net/files/
# and placed in a "models/" sub-directory next to the built executable:
#
#   models/shape_predictor_5_face_landmarks.dat       (~9.5 MB)
#   models/dlib_face_recognition_resnet_model_v1.dat  (~21 MB)
#
# If these files are absent the action shows an informative warning and exits early.
# ---------------------------------------------------------------------------

target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::MultimediaWidgets)
target_link_libraries(compendia PRIVATE Qt${QT_VERSION_MAJOR}::Svg)

# Add libexif library from 3rdParty subdirectory
target_include_directories(compendia PRIVATE ${LIBEXIF_INCLUDE_DIR})
target_link_libraries(compendia PRIVATE ${LIBEXIF_LIBRARY})

# Add libheif library from 3rdParty subdirectory
target_include_directories(compendia PRIVATE ${LIBHEIF_INCLUDE_DIR})
target_link_libraries(compendia PRIVATE ${LIBHEIF_LIBRARY})

# On Linux, force the binary to prefer the system libheif (which has decoder plugins)
# over /usr/local/lib/libheif.so.1 (built from source, no plugins).
if(UNIX AND NOT APPLE)
    set_target_properties(compendia PROPERTIES
        BUILD_RPATH "/usr/lib/x86_64-linux-gnu"
    )
endif()


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.compendia)
endif()
set_target_properties(compendia PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ── Application icon ──────────────────────────────────────────────────────────
set(ICON_SVG     "${CMAKE_CURRENT_SOURCE_DIR}/resources/compendia_icon.svg")
set(ICON_ICO     "${CMAKE_CURRENT_BINARY_DIR}/compendia.ico")   # generated here
set(ICON_ICO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/compendia.ico") # committed copy

if(WIN32)
    # Prefer Inkscape for accurate SVG rendering; fall back to ImageMagick alone.
    find_program(INKSCAPE_EXECUTABLE NAMES inkscape
        HINTS "C:/Program Files/Inkscape/bin")
    find_program(MAGICK_EXECUTABLE NAMES magick
        HINTS "C:/Program Files/ImageMagick-7.1.2-Q16-HDRI")

    set(ICON_TOOLS_FOUND FALSE)

    if(INKSCAPE_EXECUTABLE AND MAGICK_EXECUTABLE)
        # Inkscape renders each PNG size; ImageMagick assembles the ICO.
        add_custom_command(
            OUTPUT "${ICON_ICO}"
            COMMAND "${INKSCAPE_EXECUTABLE}" -w 16  -h 16  -o "${CMAKE_CURRENT_BINARY_DIR}/icon_16.png"  "${ICON_SVG}"
            COMMAND "${INKSCAPE_EXECUTABLE}" -w 32  -h 32  -o "${CMAKE_CURRENT_BINARY_DIR}/icon_32.png"  "${ICON_SVG}"
            COMMAND "${INKSCAPE_EXECUTABLE}" -w 48  -h 48  -o "${CMAKE_CURRENT_BINARY_DIR}/icon_48.png"  "${ICON_SVG}"
            COMMAND "${INKSCAPE_EXECUTABLE}" -w 256 -h 256 -o "${CMAKE_CURRENT_BINARY_DIR}/icon_256.png" "${ICON_SVG}"
            COMMAND "${MAGICK_EXECUTABLE}"
                "${CMAKE_CURRENT_BINARY_DIR}/icon_16.png"
                "${CMAKE_CURRENT_BINARY_DIR}/icon_32.png"
                "${CMAKE_CURRENT_BINARY_DIR}/icon_48.png"
                "${CMAKE_CURRENT_BINARY_DIR}/icon_256.png"
                "${ICON_ICO}"
            DEPENDS "${ICON_SVG}"
            COMMENT "Generating compendia.ico (Inkscape + ImageMagick)"
            VERBATIM
        )
        set(ICON_TOOLS_FOUND TRUE)
    elseif(MAGICK_EXECUTABLE)
        # ImageMagick renders the SVG directly and writes all ICO frames at once.
        add_custom_command(
            OUTPUT "${ICON_ICO}"
            COMMAND "${MAGICK_EXECUTABLE}"
                -background none -density 300
                "${ICON_SVG}"
                -define icon:auto-resize=256,48,32,16
                "${ICON_ICO}"
            DEPENDS "${ICON_SVG}"
            COMMENT "Generating compendia.ico (ImageMagick)"
            VERBATIM
        )
        set(ICON_TOOLS_FOUND TRUE)
    endif()

    if(ICON_TOOLS_FOUND)
        # After generating into the build dir, copy back to resources/ so the
        # result can be committed and used on machines without the tools.
        add_custom_target(compendia_ico ALL
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${ICON_ICO}" "${ICON_ICO_SRC}"
            DEPENDS "${ICON_ICO}"
            COMMENT "Copying compendia.ico to resources/"
            VERBATIM
        )
        add_dependencies(compendia compendia_ico)
    elseif(EXISTS "${ICON_ICO_SRC}")
        # No tools present — copy the checked-in ICO to the build dir at configure
        # time so windres can find it.
        message(STATUS "compendia: using checked-in resources/compendia.ico")
        configure_file("${ICON_ICO_SRC}" "${ICON_ICO}" COPYONLY)
    else()
        message(WARNING
            "compendia: Inkscape/ImageMagick not found and no checked-in ICO — "
            "Windows icon will be absent. Install either tool and rebuild.")
    endif()

    # Wire up the .rc whenever we will have an ICO in the build dir
    # (either freshly generated or copied from source at configure time).
    if(ICON_TOOLS_FOUND OR EXISTS "${ICON_ICO_SRC}")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/compendia.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/compendia.rc"
        )
        target_sources(compendia PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/compendia.rc")
    endif()
endif()

include(GNUInstallDirs)

if(UNIX AND NOT APPLE)
    # Install the scalable SVG into the hicolor icon theme so GNOME/KDE pick it up.
    install(FILES "${ICON_SVG}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
            RENAME "compendia.svg")
    # .desktop file makes the app appear in the GNOME application grid.
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/compendia.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
# ──────────────────────────────────────────────────────────────────────────────

# On Windows install the exe to the package root so windeployqt's DLLs,
# Qt plugins, and the models/ directory all sit at the same level.
if(WIN32)
    install(TARGETS compendia RUNTIME DESTINATION .)
else()
    install(TARGETS compendia
        BUNDLE  DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# ── License files ─────────────────────────────────────────────────────────────
# The project LICENSE (GPLv3) must be distributed with the binary.
# dlib is statically compiled in; its Boost Software License requires that the
# copyright notice and license text accompany all copies of the software.
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION .)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/src/dlib/LICENSE.txt"
        DESTINATION licenses
        RENAME dlib-LICENSE.txt)

# ── Model files ───────────────────────────────────────────────────────────────
# dlib face-recognition .dat files are large (~30 MB combined) and not in the
# repository.  Run  cmake -P cmake/DownloadModels.cmake  to download them.
# Once present in models/ they are bundled into every installer package.
set(MODEL_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/models/shape_predictor_5_face_landmarks.dat"
    "${CMAKE_CURRENT_SOURCE_DIR}/models/dlib_face_recognition_resnet_model_v1.dat"
)
set(MODELS_FOUND TRUE)
foreach(f IN LISTS MODEL_FILES)
    if(NOT EXISTS "${f}")
        set(MODELS_FOUND FALSE)
    endif()
endforeach()

if(MODELS_FOUND)
    install(FILES ${MODEL_FILES} DESTINATION models)
else()
    message(STATUS
        "compendia: model .dat files not found in models/ — face recognition "
        "will not work in the packaged app.\n"
        "Run:  cmake -P cmake/DownloadModels.cmake  to download them.")
endif()

# Convenience target so Qt Creator / IDEs can trigger a download from the build.
add_custom_target(download_models
    COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadModels.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Downloading dlib face recognition model files"
)

# ── Third-party DLLs (vcpkg) ──────────────────────────────────────────────────
# libexif and libheif are import-library builds from vcpkg — their DLLs and
# transitive codec dependencies must be deployed alongside the exe.
# Auto-detect the vcpkg install tree relative to this repo; override with
# -DVCPKG_INSTALLED_DIR=<path> if your vcpkg lives elsewhere.
if(WIN32)
    set(VCPKG_INSTALLED_DIR "" CACHE PATH
        "Path to vcpkg installed/x64-windows (contains bin/heif.dll etc.)")

    if(NOT VCPKG_INSTALLED_DIR)
        foreach(_candidate
                "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/installed/x64-windows"
                "$ENV{USERPROFILE}/vcpkg/installed/x64-windows"
                "C:/vcpkg/installed/x64-windows")
            if(EXISTS "${_candidate}/bin/heif.dll")
                set(VCPKG_INSTALLED_DIR "${_candidate}" CACHE PATH "" FORCE)
                break()
            endif()
        endforeach()
    endif()

    if(VCPKG_INSTALLED_DIR)
        message(STATUS "vcpkg bin: ${VCPKG_INSTALLED_DIR}/bin")
        set(_THIRD_PARTY_DLLS
            "${VCPKG_INSTALLED_DIR}/bin/exif-12.dll"
            "${VCPKG_INSTALLED_DIR}/bin/heif.dll"
            "${VCPKG_INSTALLED_DIR}/bin/libde265.dll"
            "${VCPKG_INSTALLED_DIR}/bin/libx265.dll"
        )
        install(FILES ${_THIRD_PARTY_DLLS} DESTINATION .)

        # The vcpkg DLLs above are built with MSVC and depend on the Visual C++
        # runtime.  Bundle these DLLs so the package runs on machines that do not
        # have the VC++ Redistributable installed.
        file(TO_CMAKE_PATH "$ENV{SystemRoot}" _sysroot)
        foreach(_msvcrt
                vcruntime140.dll
                vcruntime140_1.dll
                msvcp140.dll)
            if(EXISTS "${_sysroot}/System32/${_msvcrt}")
                install(FILES "${_sysroot}/System32/${_msvcrt}" DESTINATION .)
                message(STATUS "Bundling MSVC runtime: ${_msvcrt}")
            else()
                message(WARNING
                    "compendia: MSVC runtime DLL not found: ${_sysroot}/System32/${_msvcrt}\n"
                    "The package may fail on machines without the VC++ Redistributable.")
            endif()
        endforeach()

        # License files for the vcpkg-provided libraries.
        foreach(_lib libexif libheif libde265 x265 openblas fftw3)
            set(_lic "${VCPKG_INSTALLED_DIR}/share/${_lib}/copyright")
            if(EXISTS "${_lic}")
                install(FILES "${_lic}"
                        DESTINATION licenses
                        RENAME "${_lib}-LICENSE.txt")
            endif()
        endforeach()
    else()
        message(WARNING
            "compendia: vcpkg installed dir not found — exif-12.dll, heif.dll "
            "and codec DLLs will be absent from the package.\n"
            "Set -DVCPKG_INSTALLED_DIR=<path/to/vcpkg/installed/x64-windows>")
    endif()
endif()

# ── Windows Qt + runtime deployment ───────────────────────────────────────────
if(WIN32)
    # Locate windeployqt from the same Qt installation used to build.
    get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
              "C:/Qt/6.10.1/mingw_64/bin"
        NO_DEFAULT_PATH)

    # Locate the MinGW runtime DLLs that windeployqt does not copy.
    get_filename_component(QT_ROOT "${Qt6_DIR}/../../../../../" ABSOLUTE)
    file(GLOB _mingw_candidates "${QT_ROOT}/Tools/mingw*/bin")
    if(_mingw_candidates)
        list(SORT _mingw_candidates ORDER DESCENDING)
        list(GET _mingw_candidates 0 MINGW_BIN_DIR)
    else()
        set(MINGW_BIN_DIR "C:/Qt/Tools/mingw1310_64/bin")
    endif()
    message(STATUS "MinGW bin dir: ${MINGW_BIN_DIR}")

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        # install(CODE) runs as part of every cmake --install, including
        # CPack's internal install, so the DLLs end up in the package.
        install(CODE "
            message(STATUS \"Running windeployqt...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --release
                    --no-translations
                    \"\${CMAKE_INSTALL_PREFIX}/compendia.exe\"
                RESULT_VARIABLE _wd_result
            )
            if(NOT _wd_result EQUAL 0)
                message(WARNING \"windeployqt exited with code \${_wd_result}\")
            endif()

            message(STATUS \"Copying MinGW runtime DLLs...\")
            foreach(_dll
                    libgcc_s_seh-1.dll
                    libstdc++-6.dll
                    libwinpthread-1.dll)
                if(EXISTS \"${MINGW_BIN_DIR}/\${_dll}\")
                    file(COPY \"${MINGW_BIN_DIR}/\${_dll}\"
                         DESTINATION \"\${CMAKE_INSTALL_PREFIX}\")
                else()
                    message(WARNING \"MinGW DLL not found: ${MINGW_BIN_DIR}/\${_dll}\")
                endif()
            endforeach()
        ")
    else()
        message(WARNING
            "compendia: windeployqt not found — Qt DLLs will not be deployed.\n"
            "Set WINDEPLOYQT_EXECUTABLE in CMake cache or add Qt bin to PATH.")
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(compendia)
endif()

include(cmake/Packaging.cmake)
include(cmake/AppImage.cmake)
