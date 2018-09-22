include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tools
)

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/unittests/tools)

set(tools_src ${PROJECT_SOURCE_DIR}/tools)

macro(add_tools_unittest unittest extra_src)
    add_unittest(${unittest} tools/ ${extra_src})
endmacro(add_tools_unittest unittest extra_src)

add_tools_unittest(Rule_unittest $<TARGET_OBJECTS:wvmcc-parsergen-objs>)
add_tools_unittest(RuleParser_unittest $<TARGET_OBJECTS:wvmcc-parsergen-objs>)