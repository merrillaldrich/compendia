# cmake/AppImage.cmake
# Defines an 'appimage' build target that produces a self-contained
# Compendia-<version>-x86_64.AppImage in the build directory.
#
# Usage (from the repo root):
#   cmake --build <build-dir> --target appimage
#
# linuxdeploy and linuxdeploy-plugin-qt are downloaded automatically into
# <build-dir>/linuxdeploy/ on first run.

if(NOT (UNIX AND NOT APPLE))
    return()
endif()

# Detect qmake from the same Qt installation used to build.
get_filename_component(_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
find_program(QMAKE_EXECUTABLE NAMES qmake6 qmake
    HINTS "${_qt_bin_dir}"
    NO_DEFAULT_PATH)

if(NOT QMAKE_EXECUTABLE)
    set(QMAKE_EXECUTABLE "qmake6")
    message(STATUS "compendia: qmake not auto-detected; AppImage target will use 'qmake6' from PATH")
else()
    message(STATUS "qmake (for AppImage): ${QMAKE_EXECUTABLE}")
endif()

set(_linuxdeploy_dir  "${CMAKE_BINARY_DIR}/linuxdeploy")
set(_linuxdeploy_bin  "${_linuxdeploy_dir}/linuxdeploy-x86_64.AppImage")
set(_appdir           "${CMAKE_BINARY_DIR}/AppDir")
get_filename_component(_qt_lib_dir "${Qt6_DIR}/../.." ABSOLUTE)

add_custom_target(appimage
    # Step 1: Download linuxdeploy tools if not already present.
    COMMAND ${CMAKE_COMMAND}
        -DLINUXDEPLOY_DIR=${_linuxdeploy_dir}
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadLinuxDeploy.cmake"

    # Step 2: Clean AppDir from any previous run.
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${_appdir}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_appdir}"

    # Step 2b: Copy icon to a name that matches Icon= in the .desktop file.
    # linuxdeploy derives the icon name from the filename, so it must be compendia.svg.
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/compendia_icon.svg"
        "${CMAKE_BINARY_DIR}/compendia.svg"

    # Step 3: Copy dlib model files into AppDir/usr/bin/models/ so the packaged
    # app can find them via applicationDirPath() + "/models".
    # The files must already exist in <repo>/models/; run
    #   cmake -P cmake/DownloadModels.cmake   to download them first.
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_appdir}/usr/bin/models"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/models/shape_predictor_5_face_landmarks.dat"
        "${_appdir}/usr/bin/models/shape_predictor_5_face_landmarks.dat"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/models/dlib_face_recognition_resnet_model_v1.dat"
        "${_appdir}/usr/bin/models/dlib_face_recognition_resnet_model_v1.dat"

    # Step 4: Copy license files into AppDir so they are included in the image.
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        "${_appdir}/LICENSE"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_appdir}/licenses"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/3rdParty/src/dlib/LICENSE.txt"
        "${_appdir}/licenses/dlib-LICENSE.txt"

    # Step 5: Run linuxdeploy with the Qt plugin to bundle the app and produce
    # the AppImage.  The VERSION env var controls the output filename:
    #   Compendia-<version>-x86_64.AppImage
    COMMAND ${CMAKE_COMMAND} -E env
        "QMAKE=${QMAKE_EXECUTABLE}"
        "VERSION=${PROJECT_VERSION}"
        "LD_LIBRARY_PATH=${_qt_lib_dir}"
        "${_linuxdeploy_bin}"
            --appdir "${_appdir}"
            --executable "$<TARGET_FILE:compendia>"
            --desktop-file "${CMAKE_CURRENT_SOURCE_DIR}/compendia.desktop"
            --icon-file "${CMAKE_BINARY_DIR}/compendia.svg"
            --plugin qt
            --output appimage

    DEPENDS compendia
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Building AppImage for Compendia ${PROJECT_VERSION}..."
    VERBATIM
)
