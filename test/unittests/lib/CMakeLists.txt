include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_SOURCE_DIR}/include
)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/unittests/lib)

macro(add_lib_unittest unittest)
    add_unittest(${unittest} lib/)
endmacro(add_lib_unittest unittest)

add_lib_unittest(adt/Token_unittest)
add_lib_unittest(adt/Vector_unittest)
add_lib_unittest(ByteBuffer_unittest)
add_lib_unittest(FileReader_unittest)
add_lib_unittest(PassManager_unittest)
add_lib_unittest(TokenBuffer_unittest)
add_lib_unittest(Lexer_unittest)