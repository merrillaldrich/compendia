# cmake/DownloadModels.cmake
# Downloads and decompresses the two dlib face-recognition model files into
# <repo-root>/models/.  Requires Python 3 for bzip2 decompression.
#
# Usage (from the repository root):
#   cmake -P cmake/DownloadModels.cmake

set(MODELS_DIR "${CMAKE_CURRENT_LIST_DIR}/../models")
file(MAKE_DIRECTORY "${MODELS_DIR}")

set(MODEL_NAMES
    "shape_predictor_5_face_landmarks.dat"
    "dlib_face_recognition_resnet_model_v1.dat"
)
set(MODEL_URLS
    "http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2"
    "http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2"
)

find_program(PYTHON_EXECUTABLE NAMES python3 python REQUIRED)

list(LENGTH MODEL_NAMES n)
math(EXPR last "${n} - 1")

foreach(i RANGE ${last})
    list(GET MODEL_NAMES ${i} name)
    list(GET MODEL_URLS  ${i} url)

    set(dat_file "${MODELS_DIR}/${name}")
    set(bz2_file "${MODELS_DIR}/${name}.bz2")

    if(EXISTS "${dat_file}")
        message(STATUS "Already present: ${name}")
        continue()
    endif()

    message(STATUS "Downloading ${name} ...")
    file(DOWNLOAD "${url}" "${bz2_file}"
        SHOW_PROGRESS
        STATUS dl_status
    )
    list(GET dl_status 0 dl_code)
    if(NOT dl_code EQUAL 0)
        list(GET dl_status 1 dl_msg)
        message(FATAL_ERROR "Download failed for ${url}: ${dl_msg}")
    endif()

    message(STATUS "Decompressing ${name} ...")
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" -c
            "import bz2, sys; open(sys.argv[2],'wb').write(bz2.open(sys.argv[1]).read())"
            "${bz2_file}" "${dat_file}"
        RESULT_VARIABLE decomp_result
    )
    if(NOT decomp_result EQUAL 0)
        message(FATAL_ERROR "Decompression failed for ${bz2_file}")
    endif()

    file(REMOVE "${bz2_file}")
    message(STATUS "Done: ${name}")
endforeach()

message(STATUS "All model files ready in ${MODELS_DIR}")
