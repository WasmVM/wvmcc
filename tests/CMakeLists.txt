# CMake configuration for unit tests

set(TEST_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

# Make source headers available to all tests via simple includes like "pp/Tokenizer.hpp"
include_directories(${SRC_ROOT} ${TEST_ROOT}/include)

## Basic preprocessor tests (phases combined)
set(PP_BASIC_TEST ${TEST_ROOT}/unit/pp/pp_basic_test.cpp)
if(EXISTS ${PP_BASIC_TEST})
    add_executable(wvmcc-pp-basic-test ${PP_BASIC_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    target_include_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_INCLUDE_DIR})
    target_link_directories(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY_DIR})
    target_link_libraries(wvmcc-pp-basic-test PRIVATE ${WASMVM_LIBRARY})
    add_test(NAME pp-basic COMMAND wvmcc-pp-basic-test)
endif()

# Tokenizer whitespace tests
set(PP_TOKENIZER_WHITESPACE_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_whitespace_test.cpp)
if(EXISTS ${PP_TOKENIZER_WHITESPACE_TEST})
    add_executable(wvmcc-pp-tokenizer-whitespace-test ${PP_TOKENIZER_WHITESPACE_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    add_test(NAME pp-tokenizer-whitespace COMMAND wvmcc-pp-tokenizer-whitespace-test)
endif()

set(PP_TOKENIZER_PUNCT_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_punct_test.cpp)
if(EXISTS ${PP_TOKENIZER_PUNCT_TEST})
    add_executable(wvmcc-pp-tokenizer-punct-test ${PP_TOKENIZER_PUNCT_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    add_test(NAME pp-tokenizer-punct COMMAND wvmcc-pp-tokenizer-punct-test)
endif()

# Tokenizer string literal tests
set(PP_TOKENIZER_STRING_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_string_test.cpp)
if(EXISTS ${PP_TOKENIZER_STRING_TEST})
    add_executable(wvmcc-pp-tokenizer-string-test ${PP_TOKENIZER_STRING_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    add_test(NAME pp-tokenizer-string COMMAND wvmcc-pp-tokenizer-string-test)
endif()

# Tokenizer char literal tests
set(PP_TOKENIZER_CHAR_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_char_test.cpp)
if(EXISTS ${PP_TOKENIZER_CHAR_TEST})
    add_executable(wvmcc-pp-tokenizer-char-test ${PP_TOKENIZER_CHAR_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    add_test(NAME pp-tokenizer-char COMMAND wvmcc-pp-tokenizer-char-test)
endif()

# Tokenizer pp-number tests
set(PP_TOKENIZER_PPNUMBER_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_ppnumber_test.cpp)
if(EXISTS ${PP_TOKENIZER_PPNUMBER_TEST})
    add_executable(wvmcc-pp-tokenizer-ppnumber-test ${PP_TOKENIZER_PPNUMBER_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    
    add_test(NAME pp-tokenizer-ppnumber COMMAND wvmcc-pp-tokenizer-ppnumber-test)
endif()

# Tokenizer identifier tests
set(PP_TOKENIZER_IDENTIFIER_TEST ${TEST_ROOT}/unit/pp/pp_tokenizer_identifier_test.cpp)
if(EXISTS ${PP_TOKENIZER_IDENTIFIER_TEST})
    add_executable(wvmcc-pp-tokenizer-identifier-test ${PP_TOKENIZER_IDENTIFIER_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    
    add_test(NAME pp-tokenizer-identifier COMMAND wvmcc-pp-tokenizer-identifier-test)
endif()

# Lexer keyword classification test
set(LEXER_KEYWORD_TEST ${TEST_ROOT}/unit/parser/lexer_keyword_test.cpp)
if(EXISTS ${LEXER_KEYWORD_TEST})
    add_executable(wvmcc-lexer-keyword-test ${LEXER_KEYWORD_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-keyword COMMAND wvmcc-lexer-keyword-test)
endif()

# Lexer identifier classification test
set(LEXER_IDENTIFIER_TEST ${TEST_ROOT}/unit/parser/lexer_identifier_test.cpp)
if(EXISTS ${LEXER_IDENTIFIER_TEST})
    add_executable(wvmcc-lexer-identifier-test ${LEXER_IDENTIFIER_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-identifier COMMAND wvmcc-lexer-identifier-test)
endif()

# Parser translation unit test
set(PARSER_TU_TEST ${TEST_ROOT}/unit/parser/translation_unit_test.cpp)
if(EXISTS ${PARSER_TU_TEST})
    add_executable(wvmcc-parser-translation-unit-test ${PARSER_TU_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp)
    
    add_test(NAME parser-translation-unit COMMAND wvmcc-parser-translation-unit-test)
endif()
set(PARSER_SEMA_EXTERNAL_DECL_TEST ${TEST_ROOT}/unit/parser/sema_external_decl_test.cpp)
if(EXISTS ${PARSER_SEMA_EXTERNAL_DECL_TEST})
    add_executable(wvmcc-sema-external-decl-test ${PARSER_SEMA_EXTERNAL_DECL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-external-decl COMMAND wvmcc-sema-external-decl-test)
endif()

# Declaration compatibility semantic tests
set(PARSER_SEMA_DECL_COMPAT_TEST ${TEST_ROOT}/unit/parser/sema_decl_compat_test.cpp)
if(EXISTS ${PARSER_SEMA_DECL_COMPAT_TEST})
    add_executable(wvmcc-sema-decl-compat-test ${PARSER_SEMA_DECL_COMPAT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-decl-compat COMMAND wvmcc-sema-decl-compat-test)
endif()

# _Thread_local storage class checks
set(PARSER_SEMA_THREAD_LOCAL_TEST ${TEST_ROOT}/unit/parser/sema_thread_local_test.cpp)
if(EXISTS ${PARSER_SEMA_THREAD_LOCAL_TEST})
    add_executable(wvmcc-sema-thread-local-test ${PARSER_SEMA_THREAD_LOCAL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-thread-local COMMAND wvmcc-sema-thread-local-test)
endif()

# Storage-class checks for block-scope functions
set(PARSER_SEMA_STORAGE_BLOCK_TEST ${TEST_ROOT}/unit/parser/sema_storage_class_block_test.cpp)
if(EXISTS ${PARSER_SEMA_STORAGE_BLOCK_TEST})
    add_executable(wvmcc-sema-storage-block-test ${PARSER_SEMA_STORAGE_BLOCK_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-storage-block COMMAND wvmcc-sema-storage-block-test)
endif()

# Parser struct/union tests
set(PARSER_STRUCT_UNION_TEST ${TEST_ROOT}/unit/parser/struct_union_test.cpp)
if(EXISTS ${PARSER_STRUCT_UNION_TEST})
    add_executable(wvmcc-parser-struct-union-test ${PARSER_STRUCT_UNION_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/ASTPrinter.cpp)
    add_test(NAME parser-struct-union COMMAND wvmcc-parser-struct-union-test)
endif()

# Semantic struct/union checks
set(PARSER_SEMA_STRUCT_UNION_TEST ${TEST_ROOT}/unit/parser/sema_struct_union_test.cpp)
if(EXISTS ${PARSER_SEMA_STRUCT_UNION_TEST})
    add_executable(wvmcc-sema-struct-union-test ${PARSER_SEMA_STRUCT_UNION_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-struct-union COMMAND wvmcc-sema-struct-union-test)
endif()

# Parser type-specifiers test
set(PARSER_TYPE_SPEC_TEST ${TEST_ROOT}/unit/parser/type_specifiers_test.cpp)
if(EXISTS ${PARSER_TYPE_SPEC_TEST})
    add_executable(wvmcc-parser-type-spec-test ${PARSER_TYPE_SPEC_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/ASTPrinter.cpp)
    add_test(NAME parser-type-spec COMMAND wvmcc-parser-type-spec-test)
endif()

# Semantic enum checks
set(PARSER_SEMA_ENUM_TEST ${TEST_ROOT}/unit/parser/sema_enum_test.cpp)
if(EXISTS ${PARSER_SEMA_ENUM_TEST})
    add_executable(wvmcc-sema-enum-test ${PARSER_SEMA_ENUM_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-enum COMMAND wvmcc-sema-enum-test)
endif()

# Semantic atomic type checks
set(PARSER_SEMA_ATOMIC_TEST ${TEST_ROOT}/unit/parser/sema_atomic_test.cpp)
if(EXISTS ${PARSER_SEMA_ATOMIC_TEST})
    add_executable(wvmcc-sema-atomic-test ${PARSER_SEMA_ATOMIC_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-atomic COMMAND wvmcc-sema-atomic-test)
endif()

# Semantic inline/function-specifier tests
set(PARSER_SEMA_INLINE_TEST ${TEST_ROOT}/unit/parser/sema_inline_test.cpp)
if(EXISTS ${PARSER_SEMA_INLINE_TEST})
    add_executable(wvmcc-sema-inline-test ${PARSER_SEMA_INLINE_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-inline COMMAND wvmcc-sema-inline-test)
endif()

# Semantic typedef + declarator interaction tests
set(PARSER_SEMA_TYPEDEF_DECL_TEST ${TEST_ROOT}/unit/parser/sema_typedef_declarator_test.cpp)
if(EXISTS ${PARSER_SEMA_TYPEDEF_DECL_TEST})
    add_executable(wvmcc-sema-typedef-decl-test ${PARSER_SEMA_TYPEDEF_DECL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-typedef-decl COMMAND wvmcc-sema-typedef-decl-test)
endif()

    # Semantic qualifiers tests
    set(PARSER_SEMA_QUAL_TEST ${TEST_ROOT}/unit/parser/sema_qualifiers_test.cpp)
    if(EXISTS ${PARSER_SEMA_QUAL_TEST})
        add_executable(wvmcc-sema-qualifiers-test ${PARSER_SEMA_QUAL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
        add_test(NAME parser-sema-qualifiers COMMAND wvmcc-sema-qualifiers-test)
    endif()

        # Semantic restrict association tests
        set(PARSER_SEMA_RESTRICT_TEST ${TEST_ROOT}/unit/parser/sema_restrict_test.cpp)
        if(EXISTS ${PARSER_SEMA_RESTRICT_TEST})
            add_executable(wvmcc-sema-restrict-test ${PARSER_SEMA_RESTRICT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
            add_test(NAME parser-sema-restrict COMMAND wvmcc-sema-restrict-test)
        endif()

        # Semantic alignas tests
        set(PARSER_SEMA_ALIGNAS_TEST ${TEST_ROOT}/unit/parser/sema_alignas_test.cpp)
        if(EXISTS ${PARSER_SEMA_ALIGNAS_TEST})
            add_executable(wvmcc-sema-alignas-test ${PARSER_SEMA_ALIGNAS_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
            add_test(NAME parser-sema-alignas COMMAND wvmcc-sema-alignas-test)
        endif()

# Parser declarator tests
set(PARSER_DECLARATOR_TEST ${TEST_ROOT}/unit/parser/declarator_test.cpp)
if(EXISTS ${PARSER_DECLARATOR_TEST})
    add_executable(wvmcc-parser-declarator-test ${PARSER_DECLARATOR_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/ASTPrinter.cpp)
    add_test(NAME parser-declarator COMMAND wvmcc-parser-declarator-test)
endif()

# Parser initializer tests
set(PARSER_INITIALIZER_TEST ${TEST_ROOT}/unit/parser/initializer_test.cpp)
if(EXISTS ${PARSER_INITIALIZER_TEST})
    add_executable(wvmcc-parser-initializer-test ${PARSER_INITIALIZER_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp)
    add_test(NAME parser-initializer COMMAND wvmcc-parser-initializer-test)
endif()

# Initialization semantics tests
set(PARSER_SEMA_INIT_TEST ${TEST_ROOT}/unit/parser/sema_initialization_test.cpp)
if(EXISTS ${PARSER_SEMA_INIT_TEST})
    add_executable(wvmcc-sema-initialization-test ${PARSER_SEMA_INIT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-initialization COMMAND wvmcc-sema-initialization-test)
endif()

# Parser expressions tests (assignment + conditional)
set(PARSER_EXPRESSIONS_TEST ${TEST_ROOT}/unit/parser/expressions_test.cpp)
if(EXISTS ${PARSER_EXPRESSIONS_TEST})
    add_executable(wvmcc-parser-expressions-test ${PARSER_EXPRESSIONS_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp)
    add_test(NAME parser-expressions COMMAND wvmcc-parser-expressions-test)
endif()

# Parser statements tests
set(PARSER_STATEMENTS_TEST ${TEST_ROOT}/unit/parser/statements_test.cpp)
if(EXISTS ${PARSER_STATEMENTS_TEST})
    add_executable(wvmcc-parser-statements-test ${PARSER_STATEMENTS_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp)
    add_test(NAME parser-statements COMMAND wvmcc-parser-statements-test)
endif()

# Parser _Static_assert tests
set(PARSER_STATIC_ASSERT_TEST ${TEST_ROOT}/unit/parser/static_assert_test.cpp)
if(EXISTS ${PARSER_STATIC_ASSERT_TEST})
    add_executable(wvmcc-parser-static-assert-test ${PARSER_STATIC_ASSERT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp)
    add_test(NAME parser-static-assert COMMAND wvmcc-parser-static-assert-test)
endif()

# Semantic static_assert tests
set(PARSER_SEMA_STATIC_ASSERT_TEST ${TEST_ROOT}/unit/parser/sema_static_assert_test.cpp)
if(EXISTS ${PARSER_SEMA_STATIC_ASSERT_TEST})
    add_executable(wvmcc-sema-static-assert-test ${PARSER_SEMA_STATIC_ASSERT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp ${SRC_ROOT}/parser/Parser.cpp ${SRC_ROOT}/parser/ConstExprEval.cpp ${SRC_ROOT}/parser/Semantic.cpp ${SRC_ROOT}/parser/ASTVisitor.cpp)
    add_test(NAME parser-sema-static-assert COMMAND wvmcc-sema-static-assert-test)
endif()

# Lexer punctuator classification test
set(LEXER_PUNCTUATOR_TEST ${TEST_ROOT}/unit/parser/lexer_punctuator_test.cpp)
if(EXISTS ${LEXER_PUNCTUATOR_TEST})
    add_executable(wvmcc-lexer-punctuator-test ${LEXER_PUNCTUATOR_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-punctuator COMMAND wvmcc-lexer-punctuator-test)
endif()

# Lexer integer constant parsing test
set(LEXER_INTEGER_TEST ${TEST_ROOT}/unit/parser/lexer_integer_test.cpp)
if(EXISTS ${LEXER_INTEGER_TEST})
    add_executable(wvmcc-lexer-integer-test ${LEXER_INTEGER_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-integer COMMAND wvmcc-lexer-integer-test)
endif()

# Lexer floating constant parsing test
set(LEXER_FLOATING_TEST ${TEST_ROOT}/unit/parser/lexer_floating_test.cpp)
if(EXISTS ${LEXER_FLOATING_TEST})
    add_executable(wvmcc-lexer-floating-test ${LEXER_FLOATING_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-floating COMMAND wvmcc-lexer-floating-test)
endif()

# Lexer character constant parsing test
set(LEXER_CHAR_TEST ${TEST_ROOT}/unit/parser/lexer_char_test.cpp)
if(EXISTS ${LEXER_CHAR_TEST})
    add_executable(wvmcc-lexer-char-test ${LEXER_CHAR_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/parser/Lexer.cpp)
    
    add_test(NAME lexer-char COMMAND wvmcc-lexer-char-test)
endif()

# Tokenizer istream test
set(TOKENIZER_ISTREAM_TEST ${TEST_ROOT}/unit/pp/tokenizer_istream_test.cpp)
if(EXISTS ${TOKENIZER_ISTREAM_TEST})
    add_executable(wvmcc-tokenizer-istream-test ${TOKENIZER_ISTREAM_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    
    add_test(NAME tokenizer-istream COMMAND wvmcc-tokenizer-istream-test)
endif()

# Tokenizer ifstream reset test
set(TOKENIZER_IFSTREAM_RESET_TEST ${TEST_ROOT}/unit/pp/tokenizer_ifstream_reset_test.cpp)
if(EXISTS ${TOKENIZER_IFSTREAM_RESET_TEST})
    add_executable(wvmcc-tokenizer-ifstream-reset-test ${TOKENIZER_IFSTREAM_RESET_TEST} ${SRC_ROOT}/pp/Tokenizer.cpp)
    
    add_test(NAME tokenizer-ifstream-reset COMMAND wvmcc-tokenizer-ifstream-reset-test)
endif()

# Include tests (success, failure, nested combined)
set(PP_INCLUDE_TEST ${TEST_ROOT}/unit/pp/pp_include_test.cpp)
if(EXISTS ${PP_INCLUDE_TEST})
    add_executable(wvmcc-pp-include-test ${PP_INCLUDE_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-include COMMAND wvmcc-pp-include-test)
endif()

# Phase-5 normalization tests (escape decoding / UCNs)
set(PP_NORMALIZE_TEST ${TEST_ROOT}/unit/pp/pp_normalize_test.cpp)
if(EXISTS ${PP_NORMALIZE_TEST})
    add_executable(wvmcc-pp-normalize-test ${PP_NORMALIZE_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    add_test(NAME pp-normalize COMMAND wvmcc-pp-normalize-test)
endif()

# Phase-6 adjacent string literal concatenation tests (merged)
set(PP_CONCAT_TEST ${TEST_ROOT}/unit/pp/pp_concat_tests.cpp)
if(EXISTS ${PP_CONCAT_TEST})
    add_executable(wvmcc-pp-concat-test ${PP_CONCAT_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    add_test(NAME pp-concat COMMAND wvmcc-pp-concat-test)
endif()

# CLI tests: preprocess-only (-E)
set(PREPROCESS_ONLY_TEST ${TEST_ROOT}/unit/exec/preprocess_only_test.cpp)
if(EXISTS ${PREPROCESS_ONLY_TEST})
    add_executable(wvmcc-preprocess-only-test ${PREPROCESS_ONLY_TEST})
    # Ensure the main `wvmcc` executable is built before this test runs
    add_dependencies(wvmcc-preprocess-only-test wvmcc)
    add_test(NAME preprocess-only COMMAND wvmcc-preprocess-only-test)
endif()

# Macro definition tests
set(PP_MACRO_TEST ${TEST_ROOT}/unit/pp/pp_macro_test.cpp)
if(EXISTS ${PP_MACRO_TEST})
    add_executable(wvmcc-pp-macro-test ${PP_MACRO_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-macro COMMAND wvmcc-pp-macro-test)
endif()

# Conditional directive tests
set(PP_CONDITIONAL_TEST ${TEST_ROOT}/unit/pp/pp_conditional_test.cpp)
if(EXISTS ${PP_CONDITIONAL_TEST})
    add_executable(wvmcc-pp-conditional-test ${PP_CONDITIONAL_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-conditional COMMAND wvmcc-pp-conditional-test)
endif()

# Constant expression parser tests
set(PP_CONSTEXPR_TEST ${TEST_ROOT}/unit/pp/pp_constexpr_test.cpp)
if(EXISTS ${PP_CONSTEXPR_TEST})
    add_executable(wvmcc-pp-constexpr-test ${PP_CONSTEXPR_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/Tokenizer.cpp)
    
    add_test(NAME pp-constexpr COMMAND wvmcc-pp-constexpr-test)
endif()

# Utility directives tests (#error, #warning, #line, #pragma)
set(PP_DIRECTIVES_TEST ${TEST_ROOT}/unit/pp/pp_directives_test.cpp)
if(EXISTS ${PP_DIRECTIVES_TEST})
    add_executable(wvmcc-pp-directives-test ${PP_DIRECTIVES_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-directives COMMAND wvmcc-pp-directives-test)
endif()

# Dynamic __LINE__ macro tests
set(PP_LINE_MACRO_TEST ${TEST_ROOT}/unit/pp/pp_line_macro_test.cpp)
if(EXISTS ${PP_LINE_MACRO_TEST})
    add_executable(wvmcc-pp-line-macro-test ${PP_LINE_MACRO_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-line-macro COMMAND wvmcc-pp-line-macro-test)
endif()

# #pragma once tests
set(PP_PRAGMA_ONCE_TEST ${TEST_ROOT}/unit/pp/pp_pragma_once_test.cpp)
if(EXISTS ${PP_PRAGMA_ONCE_TEST})
    add_executable(wvmcc-pp-pragma-once-test ${PP_PRAGMA_ONCE_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
    
    add_test(NAME pp-pragma-once COMMAND wvmcc-pp-pragma-once-test)
endif()

# Include guard optimization tests (deferred - complex pattern detection needed)
# set(PP_INCLUDE_GUARD_TEST ${TEST_ROOT}/unit/pp_include_guard_test.cpp)
# if(EXISTS ${PP_INCLUDE_GUARD_TEST})
#     add_executable(wvmcc-pp-include-guard-test ${PP_INCLUDE_GUARD_TEST} ${SRC_ROOT}/pp/Preprocessor.cpp ${SRC_ROOT}/pp/Tokenizer.cpp ${SRC_ROOT}/pp/MacroTable.cpp ${SRC_ROOT}/pp/ConstExprParser.cpp)
#     target_include_directories(wvmcc-pp-include-guard-test PRIVATE ${SRC_ROOT} ${TEST_ROOT}/include)
#     add_test(NAME pp-include-guard COMMAND wvmcc-pp-include-guard-test)
# endif()
