# CMake configuration for unit tests

set(TEST_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

## Basic preprocessor tests (phases combined)
set(PP_BASIC_TEST ${TEST_ROOT}/unit/pp_basic_test.cpp)
if(EXISTS ${PP_BASIC_TEST})
    add_executable(wvmcc-pp-basic-test ${PP_BASIC_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp)
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

# Tokenizer istream test
set(TOKENIZER_ISTREAM_TEST ${TEST_ROOT}/unit/tokenizer_istream_test.cpp)
if(EXISTS ${TOKENIZER_ISTREAM_TEST})
    add_executable(wvmcc-tokenizer-istream-test ${TOKENIZER_ISTREAM_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-tokenizer-istream-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME tokenizer-istream COMMAND wvmcc-tokenizer-istream-test)
endif()

# Tokenizer ifstream reset test
set(TOKENIZER_IFSTREAM_RESET_TEST ${TEST_ROOT}/unit/tokenizer_ifstream_reset_test.cpp)
if(EXISTS ${TOKENIZER_IFSTREAM_RESET_TEST})
    add_executable(wvmcc-tokenizer-ifstream-reset-test ${TOKENIZER_IFSTREAM_RESET_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    target_include_directories(wvmcc-tokenizer-ifstream-reset-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME tokenizer-ifstream-reset COMMAND wvmcc-tokenizer-ifstream-reset-test)
endif()

# Include tests (success, failure, nested combined)
set(PP_INCLUDE_TEST ${TEST_ROOT}/unit/pp_include_test.cpp)
if(EXISTS ${PP_INCLUDE_TEST})
    add_executable(wvmcc-pp-include-test ${PP_INCLUDE_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp)
    target_include_directories(wvmcc-pp-include-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-include COMMAND wvmcc-pp-include-test)
endif()

# Macro definition tests
set(PP_MACRO_TEST ${TEST_ROOT}/unit/pp_macro_test.cpp)
if(EXISTS ${PP_MACRO_TEST})
    add_executable(wvmcc-pp-macro-test ${PP_MACRO_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp)
    target_include_directories(wvmcc-pp-macro-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-macro COMMAND wvmcc-pp-macro-test)
endif()

# Conditional directive tests
set(PP_CONDITIONAL_TEST ${TEST_ROOT}/unit/pp_conditional_test.cpp)
if(EXISTS ${PP_CONDITIONAL_TEST})
    add_executable(wvmcc-pp-conditional-test ${PP_CONDITIONAL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp)
    target_include_directories(wvmcc-pp-conditional-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
    add_test(NAME pp-conditional COMMAND wvmcc-pp-conditional-test)
endif()
