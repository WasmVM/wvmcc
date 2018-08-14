cmake_minimum_required(VERSION 3.5)
project(wvmcc)
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Werror")
set(CMAKE_DEBUG_POSTFIX "_d")
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)
file(READ ${CMAKE_CURRENT_LIST_DIR}/VERSION WVMCC_VERSION)
add_definitions("-DWVMCC_VERSION=\"${WVMCC_VERSION}\"")

add_subdirectory(src)

# SkyPat
option(ENABLE_TESTS OFF)
if(ENABLE_TESTS)
find_package(SkyPat REQUIRED)
add_subdirectory(test)
endif(ENABLE_TESTS)