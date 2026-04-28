set(TEST_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)

include_directories(${SRC_ROOT} ${TEST_ROOT}/include)

add_subdirectory(unit/pp)
add_subdirectory(unit/parser)
add_subdirectory(unit/exec)
add_subdirectory(unit/codegen)
