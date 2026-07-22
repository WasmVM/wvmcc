cmake_minimum_required(VERSION 3.16)
project(wvmcc VERSION 1.0.0)
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Optional: enable sanitizers (AddressSanitizer + UBSAN) for debugging
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(ENABLE_SANITIZERS)
    message(STATUS "Building with ASAN and UBSAN enabled")
    if(NOT CMAKE_CXX_FLAGS MATCHES "-fsanitize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer -g")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer -g")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
    endif()
endif()

# Find WasmVM dependency
find_package(WasmVM REQUIRED)

# Project layout
set(SRC_ROOT ${PROJECT_ROOT}/src)
file(GLOB_RECURSE WVMCC_SOURCES
	${SRC_ROOT}/exec/*.cpp
    ${SRC_ROOT}/pp/*.cpp
    ${SRC_ROOT}/parser/*.cpp
    ${SRC_ROOT}/codegen/*.cpp
    ${SRC_ROOT}/link/*.cpp
)

add_executable(wvmcc ${WVMCC_SOURCES})
target_include_directories(wvmcc PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc PRIVATE ${WASMVM_LIBRARY})
target_compile_definitions(wvmcc PRIVATE WVMCC_VERSION="${PROJECT_VERSION}")
# The installed binary must still find the WasmVM shared library (its
# install_name is @rpath/libWasmVM.dylib on macOS) — keep the lib dir as an
# RPATH after install stripping.
set_target_properties(wvmcc PROPERTIES INSTALL_RPATH "${WASMVM_LIBRARY_DIR}")

install(TARGETS wvmcc RUNTIME DESTINATION bin COMPONENT programs)

# ============================================
# Runtime (libc) — M2-18
# ============================================
option(WVMCC_BUILD_RUNTIME "Build the libc runtime" ON)
if(WVMCC_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

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

# Convenience targets for the C17 standard-conformance suite (need `wasmvm` on
# PATH for the `run`/`stdout` rows).
#
#   check-supported — the GATE: every status-supported row must pass. Exits
#                     non-zero on any failure, so it is usable as a hard check.
#   check-partial   — INFORMATIONAL: runs the status-partial rows (intentionally
#                     incomplete features) and never fails the build. A row that
#                     now PASSES here is a promotion candidate — flip it to
#                     `supported` in tests/standard/scaffold/*.cmake + docs/standard/.
add_custom_target(check-supported
    COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}
            -L status-supported --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
    VERBATIM
    COMMENT "Running status-supported conformance tests (GATE — must be 100%)...")

add_custom_target(check-partial
    COMMAND sh -c "${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -L status-partial --output-on-failure; exit 0"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    USES_TERMINAL
    VERBATIM
    COMMENT "status-partial tests (informational; never fails — a PASS here is a promotion candidate)...")
# ============================================
# Packaging (CPack) — see RELEASING.md
# ============================================
include(${CMAKE_CURRENT_LIST_DIR}/cmake/pack.cmake)
