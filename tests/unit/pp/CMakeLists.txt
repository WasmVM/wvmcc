set(SRC ${PROJECT_SOURCE_DIR}/src)

set(PP_SRCS
    ${SRC}/pp/Preprocessor.cpp
    ${SRC}/pp/Tokenizer.cpp
    ${SRC}/pp/MacroTable.cpp
    ${SRC}/pp/ConstExprParser.cpp
)

add_executable(wvmcc-pp-basic-test pp_basic_test.cpp ${PP_SRCS})
target_include_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_INCLUDE_DIR})
target_link_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY_DIR})
target_link_libraries(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY})
add_test(NAME pp-basic COMMAND wvmcc-pp-basic-test)

add_executable(wvmcc-pp-tokenizer-whitespace-test pp_tokenizer_whitespace_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-whitespace COMMAND wvmcc-pp-tokenizer-whitespace-test)

add_executable(wvmcc-pp-tokenizer-punct-test pp_tokenizer_punct_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-punct COMMAND wvmcc-pp-tokenizer-punct-test)

add_executable(wvmcc-pp-tokenizer-string-test pp_tokenizer_string_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-string COMMAND wvmcc-pp-tokenizer-string-test)

add_executable(wvmcc-pp-tokenizer-char-test pp_tokenizer_char_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-char COMMAND wvmcc-pp-tokenizer-char-test)

add_executable(wvmcc-pp-tokenizer-ppnumber-test pp_tokenizer_ppnumber_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-ppnumber COMMAND wvmcc-pp-tokenizer-ppnumber-test)

add_executable(wvmcc-pp-tokenizer-identifier-test pp_tokenizer_identifier_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME pp-tokenizer-identifier COMMAND wvmcc-pp-tokenizer-identifier-test)

add_executable(wvmcc-tokenizer-istream-test tokenizer_istream_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME tokenizer-istream COMMAND wvmcc-tokenizer-istream-test)

add_executable(wvmcc-tokenizer-ifstream-reset-test tokenizer_ifstream_reset_test.cpp ${SRC}/pp/Tokenizer.cpp)
add_test(NAME tokenizer-ifstream-reset COMMAND wvmcc-tokenizer-ifstream-reset-test)

add_executable(wvmcc-pp-include-test pp_include_test.cpp ${PP_SRCS})
add_test(NAME pp-include COMMAND wvmcc-pp-include-test)

add_executable(wvmcc-pp-normalize-test pp_normalize_test.cpp ${PP_SRCS})
add_test(NAME pp-normalize COMMAND wvmcc-pp-normalize-test)

add_executable(wvmcc-pp-concat-test pp_concat_tests.cpp ${PP_SRCS})
add_test(NAME pp-concat COMMAND wvmcc-pp-concat-test)

add_executable(wvmcc-pp-macro-test pp_macro_test.cpp ${PP_SRCS})
add_test(NAME pp-macro COMMAND wvmcc-pp-macro-test)

add_executable(wvmcc-pp-conditional-test pp_conditional_test.cpp ${PP_SRCS})
add_test(NAME pp-conditional COMMAND wvmcc-pp-conditional-test)

add_executable(wvmcc-pp-constexpr-test pp_constexpr_test.cpp ${PP_SRCS})
add_test(NAME pp-constexpr COMMAND wvmcc-pp-constexpr-test)

add_executable(wvmcc-pp-directives-test pp_directives_test.cpp ${PP_SRCS})
add_test(NAME pp-directives COMMAND wvmcc-pp-directives-test)

add_executable(wvmcc-pp-line-macro-test pp_line_macro_test.cpp ${PP_SRCS})
add_test(NAME pp-line-macro COMMAND wvmcc-pp-line-macro-test)

add_executable(wvmcc-pp-pragma-once-test pp_pragma_once_test.cpp ${PP_SRCS})
add_test(NAME pp-pragma-once COMMAND wvmcc-pp-pragma-once-test)
