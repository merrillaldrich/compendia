cmake_minimum_required(VERSION 3.16)

project(luminism VERSION 0.1 LANGUAGES CXX)

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

find_package(dlib)

#find_package(libexif REQUIRED) # this doesn't work
#find_package(libheif)

# libexif does not provide CMAKE config, so link straight to files
set(LIBEXIF_INCLUDE_DIR "3rdParty/src/libexif")
set(LIBEXIF_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/lib/exif.lib") # or libexif.so / libexif.lib

# libheif does not provide CMAKE config, so link straight to files
set(LIBHEIF_INCLUDE_DIR "3rdParty/src/libheif")
set(LIBHEIF_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/lib/heif.lib") # or .so / .lib

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(luminism
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        tagset.cpp
        tag.cpp tagfamily.cpp taggedfile.cpp
        luminismcore.h tagfamily.h taggedfile.h  tagset.h
        luminismcore.cpp
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

    )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET luminism APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(luminism SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(luminism
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Add dlib library from 3rdParty subdirectory
add_subdirectory(3rdParty/src/dlib)
target_link_libraries(luminism PRIVATE dlib)

# facerecognizer.cpp instantiates dlib's DNN templates, which generate an object
# file with more than 65 535 COFF sections on Windows — exceeding the default
# 32-bit PE/COFF limit.  Enable the extended ("big") object format for that
# single translation unit to avoid the "file too big" assembler error.
if (MSVC)
    set_source_files_properties(facerecognizer.cpp PROPERTIES COMPILE_OPTIONS "/bigobj;/O2;/arch:AVX2")
else()
    # -Wa,-mbig-obj : switch the assembler to extended COFF (section count > 65535)
    # -O2           : aggressive inlining collapses per-instantiation functions,
    #                 drastically reducing the number of .xdata$ COMDAT sections
    # -g0           : disable debug-info emission for this file so the section-name
    #                 string table stays well under the GNU assembler's 268 MB limit
    # -mavx2 -mfma  : enable 256-bit AVX2 and fused multiply-add so dlib's DNN
    #                 convolutions use SIMD rather than scalar code (~5-8x faster)
    set_source_files_properties(facerecognizer.cpp PROPERTIES COMPILE_OPTIONS "-Wa,-mbig-obj;-O2;-g0;-mavx2;-mfma")
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

target_link_libraries(luminism PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(luminism PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(luminism PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent)
target_link_libraries(luminism PRIVATE Qt${QT_VERSION_MAJOR}::Multimedia)
target_link_libraries(luminism PRIVATE Qt${QT_VERSION_MAJOR}::MultimediaWidgets)

# Add libexif library from 3rdParty subdirectory
target_include_directories(luminism PRIVATE ${LIBEXIF_INCLUDE_DIR})
target_link_libraries(luminism PRIVATE ${LIBEXIF_LIBRARY})

# Add libheif library from 3rdParty subdirectory
target_include_directories(luminism PRIVATE ${LIBHEIF_INCLUDE_DIR})
target_link_libraries(luminism PRIVATE ${LIBHEIF_LIBRARY})


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.luminism)
endif()
set_target_properties(luminism PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS luminism
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(luminism)
endif()
