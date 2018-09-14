include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/exec
)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/unittests/exec)

set(exec_src ${PROJECT_SOURCE_DIR}/exec)

macro(add_exec_unittest unittest extra_src)
    add_unittest(${unittest} exec/ ${extra_src})
endmacro(add_exec_unittest unittest extra_src)

add_exec_unittest(CCOption_unittest ${exec_src}/CCOption.c)
