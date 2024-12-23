cmake_minimum_required(VERSION 3.16)
project(wvmcc VERSION 0.0.1)
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(CONSOLE_COLOR ON CACHE BOOL "Turn on color in console result")
if(CONSOLE_COLOR)
  add_compile_definitions(CONSOLE_COLOR)
endif(CONSOLE_COLOR)

find_package(WasmVM REQUIRED)
include_directories(${WASMVM_INCLUDE_DIR})
link_directories(${WASMVM_LIBRARY_DIR})

if(UNIX)
  set(DEFAULT_LOOKUP_PATH "/usr/local" CACHE PATH "Default lookup path")
elseif(WIN32)
  set(DEFAULT_LOOKUP_PATH "C:\\Program Files\\${PROJECT_NAME}" CACHE PATH "Default lookup path")
endif(UNIX)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src/include)


add_subdirectory(external)
link_directories(${PROJECT_BINARY_DIR}/external/antlr4_runtime/src/antlr4_runtime-install/lib)
add_subdirectory(src)

set(BUILD_TESTS ON CACHE BOOL "Build tests")
if(BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif(BUILD_TESTS)