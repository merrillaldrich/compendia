# cmake/DownloadLinuxDeploy.cmake
# Downloads linuxdeploy and linuxdeploy-plugin-qt into LINUXDEPLOY_DIR.
# Called automatically by the 'appimage' build target; can also be run manually:
#
#   cmake -DLINUXDEPLOY_DIR=/path/to/dir -P cmake/DownloadLinuxDeploy.cmake

if(NOT LINUXDEPLOY_DIR)
    message(FATAL_ERROR "LINUXDEPLOY_DIR must be set. Pass -DLINUXDEPLOY_DIR=<path>")
endif()

file(MAKE_DIRECTORY "${LINUXDEPLOY_DIR}")

set(TOOL_NAMES
    "linuxdeploy-x86_64.AppImage"
    "linuxdeploy-plugin-qt-x86_64.AppImage"
)
set(TOOL_URLS
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
)

list(LENGTH TOOL_NAMES n)
math(EXPR last "${n} - 1")

foreach(i RANGE ${last})
    list(GET TOOL_NAMES ${i} tool)
    list(GET TOOL_URLS  ${i} url)
    set(dest "${LINUXDEPLOY_DIR}/${tool}")

    if(EXISTS "${dest}")
        message(STATUS "Already present: ${tool}")
        continue()
    endif()

    message(STATUS "Downloading ${tool} ...")
    file(DOWNLOAD "${url}" "${dest}"
        SHOW_PROGRESS
        STATUS dl_status
    )
    list(GET dl_status 0 dl_code)
    if(NOT dl_code EQUAL 0)
        list(GET dl_status 1 dl_msg)
        message(FATAL_ERROR "Download failed for ${url}: ${dl_msg}")
    endif()

    execute_process(COMMAND chmod +x "${dest}")
    message(STATUS "Downloaded: ${tool}")
endforeach()

message(STATUS "linuxdeploy tools ready in ${LINUXDEPLOY_DIR}")
