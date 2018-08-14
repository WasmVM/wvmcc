cmake_minimum_required(VERSION 3.5)
project(wvcc)
set(CMAKE_C_STANDARD "11")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Werror")
set(CMAKE_DEBUG_POSTFIX "_d")

add_subdirectory(src)

# SkyPat
option(ENABLE_TESTS OFF)
if(ENABLE_TESTS)
find_package(SkyPat REQUIRED)
add_subdirectory(test)
endif(ENABLE_TESTS)