cmake_minimum_required(VERSION 3.16)
project(wvmcc VERSION 0.0.1)
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Find WasmVM dependency
find_package(WasmVM REQUIRED)

# Project layout
set(SRC_ROOT ${PROJECT_ROOT}/src)
file(GLOB_RECURSE WVMCC_SOURCES
	${SRC_ROOT}/exec/*.cpp
	${SRC_ROOT}/pp/*.cpp
)

add_executable(wvmcc ${WVMCC_SOURCES})
target_include_directories(wvmcc PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc PRIVATE ${WASMVM_LIBRARY})

install(TARGETS wvmcc RUNTIME DESTINATION bin)

# Tests
enable_testing()
add_subdirectory(tests)