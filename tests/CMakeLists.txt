# CMake configuration for unit tests

set(TEST_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

## Basic preprocessor tests (phases combined)
set(PP_BASIC_TEST ${TEST_ROOT}/unit/pp_basic_test.cpp)
if(EXISTS ${PP_BASIC_TEST})
    add_executable(wvmcc-pp-basic-test ${PP_BASIC_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_INCLUDE_DIR} ${SRC_ROOT})
    target_link_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-basic COMMAND wvmcc-pp-basic-test)
endif()

# Tokenizer whitespace tests
set(PP_TOKENIZER_WHITESPACE_TEST ${TEST_ROOT}/unit/pp_tokenizer_whitespace_test.cpp)
if(EXISTS ${PP_TOKENIZER_WHITESPACE_TEST})
    add_executable(wvmcc-pp-tokenizer-whitespace-test ${PP_TOKENIZER_WHITESPACE_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-whitespace-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-whitespace COMMAND wvmcc-pp-tokenizer-whitespace-test)
endif()

set(PP_TOKENIZER_PUNCT_TEST ${TEST_ROOT}/unit/pp_tokenizer_punct_test.cpp)
if(EXISTS ${PP_TOKENIZER_PUNCT_TEST})
    add_executable(wvmcc-pp-tokenizer-punct-test ${PP_TOKENIZER_PUNCT_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-punct-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-punct COMMAND wvmcc-pp-tokenizer-punct-test)
endif()

# Tokenizer string literal tests
set(PP_TOKENIZER_STRING_TEST ${TEST_ROOT}/unit/pp_tokenizer_string_test.cpp)
if(EXISTS ${PP_TOKENIZER_STRING_TEST})
    add_executable(wvmcc-pp-tokenizer-string-test ${PP_TOKENIZER_STRING_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-string-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-string COMMAND wvmcc-pp-tokenizer-string-test)
endif()

# Tokenizer char literal tests
set(PP_TOKENIZER_CHAR_TEST ${TEST_ROOT}/unit/pp_tokenizer_char_test.cpp)
if(EXISTS ${PP_TOKENIZER_CHAR_TEST})
    add_executable(wvmcc-pp-tokenizer-char-test ${PP_TOKENIZER_CHAR_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-char-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-char COMMAND wvmcc-pp-tokenizer-char-test)
endif()

# Tokenizer pp-number tests
set(PP_TOKENIZER_PPNUMBER_TEST ${TEST_ROOT}/unit/pp_tokenizer_ppnumber_test.cpp)
if(EXISTS ${PP_TOKENIZER_PPNUMBER_TEST})
    add_executable(wvmcc-pp-tokenizer-ppnumber-test ${PP_TOKENIZER_PPNUMBER_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-ppnumber-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-ppnumber COMMAND wvmcc-pp-tokenizer-ppnumber-test)
endif()

# Tokenizer identifier tests
set(PP_TOKENIZER_IDENTIFIER_TEST ${TEST_ROOT}/unit/pp_tokenizer_identifier_test.cpp)
if(EXISTS ${PP_TOKENIZER_IDENTIFIER_TEST})
    add_executable(wvmcc-pp-tokenizer-identifier-test ${PP_TOKENIZER_IDENTIFIER_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-pp-tokenizer-identifier-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-tokenizer-identifier COMMAND wvmcc-pp-tokenizer-identifier-test)
endif()
