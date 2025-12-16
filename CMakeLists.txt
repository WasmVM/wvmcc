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

# ============================================
# Code Complexity Monitoring
# ============================================

# Find Python for complexity tools
find_package(Python3 COMPONENTS Interpreter)

if(Python3_FOUND)
    # Check if lizard is installed via pipx or pip
    execute_process(
        COMMAND lizard --version
        RESULT_VARIABLE LIZARD_CHECK
        OUTPUT_QUIET ERROR_QUIET
    )
    
    if(LIZARD_CHECK EQUAL 0)
        message(STATUS "Lizard complexity analyzer found")
        set(LIZARD_AVAILABLE TRUE)
    else()
        message(STATUS "Lizard not found. Install with: pipx install lizard")
        set(LIZARD_AVAILABLE FALSE)
    endif()
    
    # Complexity analysis target
    if(LIZARD_AVAILABLE)
        add_custom_target(complexity
            COMMAND lizard 
                ${SRC_ROOT}
                -l cpp
                --sort cyclomatic_complexity
                -w
            WORKING_DIRECTORY ${PROJECT_ROOT}
            COMMENT "Analyzing code complexity with Lizard..."
        )
        
        # Detailed complexity report
        add_custom_target(complexity-report
            COMMAND lizard 
                ${SRC_ROOT}
                -l cpp
                --sort cyclomatic_complexity
                -w
                -o ${CMAKE_BINARY_DIR}/complexity-report.html
                --html
            WORKING_DIRECTORY ${PROJECT_ROOT}
            COMMENT "Generating HTML complexity report..."
        )
        
        # Complexity check with thresholds
        add_custom_target(complexity-check
            COMMAND lizard 
                ${SRC_ROOT}
                -l cpp
                -C 15
                -L 1000
                -a 5
                -w
            WORKING_DIRECTORY ${PROJECT_ROOT}
            COMMENT "Checking complexity thresholds (CCN<15, Length<1000, Args<5)..."
        )
    endif()
endif()

# Tests
enable_testing()
add_subdirectory(tests)