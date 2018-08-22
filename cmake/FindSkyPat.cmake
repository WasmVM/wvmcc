if(SKYPAT_PATH)
set(CMAKE_FIND_ROOT_PATH ${SKYPAT_PATH} ${CMAKE_FIND_ROOT_PATH})
endif(SKYPAT_PATH)
find_library(SKYPAT_LIBRARIES
    NAMES skypat
)
find_path(SKYPAT_INCLUDE_DIR
    NAMES skypat/skypat.h
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SKYPAT REQUIRED_VARS SKYPAT_INCLUDE_DIR SKYPAT_LIBRARIES)