# CMake configuration for unit tests

set(TEST_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

# Phase 1 unit test
set(PP_PHASE1_TEST ${TEST_ROOT}/unit/pp_phase1_test.cpp)
if(EXISTS ${PP_PHASE1_TEST})
    add_executable(wvmcc-pp-phase1-test ${PP_PHASE1_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-phase1-test PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
    target_link_directories(wvmcc-pp-phase1-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-phase1-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-phase1 COMMAND wvmcc-pp-phase1-test)
endif()

# Phase 2 unit test
set(PP_PHASE2_TEST ${TEST_ROOT}/unit/pp_phase2_test.cpp)
if(EXISTS ${PP_PHASE2_TEST})
    add_executable(wvmcc-pp-phase2-test ${PP_PHASE2_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-phase2-test PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
    target_link_directories(wvmcc-pp-phase2-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-phase2-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-phase2 COMMAND wvmcc-pp-phase2-test)
endif()

# Phase 3 unit test
set(PP_PHASE3_TEST ${TEST_ROOT}/unit/pp_phase3_test.cpp)
if(EXISTS ${PP_PHASE3_TEST})
    add_executable(wvmcc-pp-phase3-test ${PP_PHASE3_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-phase3-test PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
    target_link_directories(wvmcc-pp-phase3-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-phase3-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-phase3 COMMAND wvmcc-pp-phase3-test)
endif()

# Combined integration test
set(PP_PHASE_COMBINED_TEST ${TEST_ROOT}/unit/pp_phase_combined_test.cpp)
if(EXISTS ${PP_PHASE_COMBINED_TEST})
    add_executable(wvmcc-pp-phase-combined-test ${PP_PHASE_COMBINED_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-phase-combined-test PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
    target_link_directories(wvmcc-pp-phase-combined-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-phase-combined-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-phase-combined COMMAND wvmcc-pp-phase-combined-test)
endif()

# Tokenizer minimal tests
set(PP_TOKENIZER_MIN_TEST ${TEST_ROOT}/unit/pp_tokenizer_min_test.cpp)
if(EXISTS ${PP_TOKENIZER_MIN_TEST})
    add_executable(wvmcc-pp-tokenizer-min-test ${PP_TOKENIZER_MIN_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-min-test PRIVATE ${SRC_ROOT})
    add_test(NAME pp-tokenizer-min COMMAND wvmcc-pp-tokenizer-min-test)
endif()
