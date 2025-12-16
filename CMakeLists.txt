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
set(TEST_ROOT ${PROJECT_ROOT}/tests)
file(GLOB_RECURSE WVMCC_TEST_SOURCES
	${TEST_ROOT}/unit/*.cpp
)
if(WVMCC_TEST_SOURCES)
	# Include implementation sources needed by tests (e.g., preprocessor)
	file(GLOB_RECURSE WVMCC_TEST_IMPL_SOURCES
		${SRC_ROOT}/pp/*.cpp
	)
	add_executable(wvmcc-tests ${WVMCC_TEST_SOURCES} ${WVMCC_TEST_IMPL_SOURCES})
	target_include_directories(wvmcc-tests PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
	target_link_directories(wvmcc-tests PRIVATE ${WASMVM_LIBRARY_DIR})
	target_link_libraries(wvmcc-tests PRIVATE ${WASMVM_LIBRARY})
	add_test(NAME unit-tests COMMAND wvmcc-tests)
endif()