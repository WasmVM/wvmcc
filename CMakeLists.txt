cmake_minimum_required(VERSION 3.16)
project(wvmcc)
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)

# LLVM
include(ExternalProject)
ExternalProject_Add(llvm
  GIT_REPOSITORY    https://github.com/llvm/llvm-project.git
  GIT_TAG           llvmorg-17.0.5
  GIT_SHALLOW       ON
  GIT_PROGRESS      ON
  CMAKE_ARGS        -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/llvm-install -DLLVM_TARGETS_TO_BUILD=WebAssembly -DLLVM_BUILD_EXAMPLES=OFF -DLLVM_BUILD_TESTS=OFF -DLLVM_BUILD_TOOLS=OFF -DBENCHMARK_INSTALL_DOCS=OFF -DCLANG_BUILD_TOOLS=OFF -DCLANG_INCLUDE_DOCS=OFF -DCLANG_INCLUDE_TESTS=OFF -DCLANG_PLUGIN_SUPPORT=OFF -DLLVM_BUILD_RUNTIME=OFF -DLLVM_BUILD_RUNTIMES=OFF -DLLVM_BUILD_UTILS=OFF
  SOURCE_SUBDIR     llvm
)
include_directories(${CMAKE_BINARY_DIR}/llvm-install/include)
link_directories(${CMAKE_BINARY_DIR}/llvm-install/lib)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src/include)
add_subdirectory(src)

set(BUILD_TESTS ON CACHE BOOL "Build tests")
if(BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif(BUILD_TESTS)