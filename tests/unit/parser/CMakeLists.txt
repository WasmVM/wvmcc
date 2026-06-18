set(SRC ${PROJECT_SOURCE_DIR}/src)

set(PP_SRCS
    ${SRC}/pp/Preprocessor.cpp
    ${SRC}/pp/Tokenizer.cpp
    ${SRC}/pp/MacroTable.cpp
    ${SRC}/pp/ConstExprParser.cpp
)

set(LEXER_SRCS ${PP_SRCS} ${SRC}/parser/Lexer.cpp)

set(PARSER_SRCS ${LEXER_SRCS}
    ${SRC}/parser/Parser.cpp
    ${SRC}/parser/ConstExprEval.cpp
)

set(SEMA_SRCS ${PARSER_SRCS}
    ${SRC}/parser/Semantic.cpp
    ${SRC}/parser/ASTVisitor.cpp
)

# Lexer tests

add_executable(wvmcc-lexer-keyword-test lexer_keyword_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-keyword COMMAND wvmcc-lexer-keyword-test)

add_executable(wvmcc-lexer-identifier-test lexer_identifier_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-identifier COMMAND wvmcc-lexer-identifier-test)

add_executable(wvmcc-lexer-punctuator-test lexer_punctuator_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-punctuator COMMAND wvmcc-lexer-punctuator-test)

add_executable(wvmcc-lexer-integer-test lexer_integer_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-integer COMMAND wvmcc-lexer-integer-test)

add_executable(wvmcc-lexer-floating-test lexer_floating_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-floating COMMAND wvmcc-lexer-floating-test)

add_executable(wvmcc-lexer-char-test lexer_char_test.cpp ${LEXER_SRCS})
add_test(NAME lexer-char COMMAND wvmcc-lexer-char-test)

# Parser tests

add_executable(wvmcc-parser-translation-unit-test translation_unit_test.cpp ${PARSER_SRCS})
add_test(NAME parser-translation-unit COMMAND wvmcc-parser-translation-unit-test)

add_executable(wvmcc-parser-struct-union-test struct_union_test.cpp ${PARSER_SRCS} ${SRC}/parser/ASTPrinter.cpp)
add_test(NAME parser-struct-union COMMAND wvmcc-parser-struct-union-test)

add_executable(wvmcc-parser-type-spec-test type_specifiers_test.cpp ${PARSER_SRCS} ${SRC}/parser/ASTPrinter.cpp)
add_test(NAME parser-type-spec COMMAND wvmcc-parser-type-spec-test)

add_executable(wvmcc-parser-declarator-test declarator_test.cpp ${PARSER_SRCS} ${SRC}/parser/ASTPrinter.cpp)
add_test(NAME parser-declarator COMMAND wvmcc-parser-declarator-test)

add_executable(wvmcc-parser-initializer-test initializer_test.cpp ${PARSER_SRCS})
add_test(NAME parser-initializer COMMAND wvmcc-parser-initializer-test)

add_executable(wvmcc-parser-expressions-test expressions_test.cpp ${PARSER_SRCS})
add_test(NAME parser-expressions COMMAND wvmcc-parser-expressions-test)

add_executable(wvmcc-parser-statements-test statements_test.cpp ${PARSER_SRCS})
add_test(NAME parser-statements COMMAND wvmcc-parser-statements-test)

add_executable(wvmcc-parser-static-assert-test static_assert_test.cpp ${PARSER_SRCS})
add_test(NAME parser-static-assert COMMAND wvmcc-parser-static-assert-test)

# Regression: parser must not hang on the #92 constructs (timeout backstops a
# reintroduced infinite loop).
add_executable(wvmcc-parser-no-hang-test parser_no_hang_test.cpp ${PARSER_SRCS})
add_test(NAME parser-no-hang COMMAND wvmcc-parser-no-hang-test)
set_tests_properties(parser-no-hang PROPERTIES TIMEOUT 30)

add_executable(wvmcc-parser-gnu-attribute-test gnu_attribute_test.cpp ${PARSER_SRCS})
add_test(NAME parser-gnu-attribute COMMAND wvmcc-parser-gnu-attribute-test)

# Semantic tests

add_executable(wvmcc-sema-external-decl-test sema_external_decl_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-external-decl COMMAND wvmcc-sema-external-decl-test)

add_executable(wvmcc-sema-decl-compat-test sema_decl_compat_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-decl-compat COMMAND wvmcc-sema-decl-compat-test)

add_executable(wvmcc-sema-thread-local-test sema_thread_local_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-thread-local COMMAND wvmcc-sema-thread-local-test)

add_executable(wvmcc-sema-storage-block-test sema_storage_class_block_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-storage-block COMMAND wvmcc-sema-storage-block-test)

add_executable(wvmcc-sema-struct-union-test sema_struct_union_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-struct-union COMMAND wvmcc-sema-struct-union-test)

add_executable(wvmcc-sema-enum-test sema_enum_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-enum COMMAND wvmcc-sema-enum-test)

add_executable(wvmcc-sema-atomic-test sema_atomic_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-atomic COMMAND wvmcc-sema-atomic-test)

add_executable(wvmcc-sema-inline-test sema_inline_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-inline COMMAND wvmcc-sema-inline-test)

add_executable(wvmcc-sema-typedef-decl-test sema_typedef_declarator_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-typedef-decl COMMAND wvmcc-sema-typedef-decl-test)

add_executable(wvmcc-sema-qualifiers-test sema_qualifiers_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-qualifiers COMMAND wvmcc-sema-qualifiers-test)

add_executable(wvmcc-sema-restrict-test sema_restrict_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-restrict COMMAND wvmcc-sema-restrict-test)

add_executable(wvmcc-sema-alignas-test sema_alignas_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-alignas COMMAND wvmcc-sema-alignas-test)

add_executable(wvmcc-sema-initialization-test sema_initialization_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-initialization COMMAND wvmcc-sema-initialization-test)

add_executable(wvmcc-sema-static-assert-test sema_static_assert_test.cpp ${SEMA_SRCS})
add_test(NAME parser-sema-static-assert COMMAND wvmcc-sema-static-assert-test)
