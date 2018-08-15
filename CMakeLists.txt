cmake_minimum_required(VERSION 3.5)
project(wvmcc)
set(CMAKE_C_STANDARD "11")
set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Werror")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Werror")
set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

# version
file(READ ${CMAKE_CURRENT_LIST_DIR}/VERSION WVMCC_VERSION)
add_definitions("-DWVMCC_VERSION=\"${WVMCC_VERSION}\"")

# src
add_subdirectory(src)

# test
option(ENABLE_TESTS "Enable test" ON)
if(ENABLE_TESTS)
    find_package(SkyPat REQUIRED)
    include_directories(${SKYPAT_INCLUDE_DIR})
    add_subdirectory(test)
endif(ENABLE_TESTS)