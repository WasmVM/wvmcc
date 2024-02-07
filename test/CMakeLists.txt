include_directories(${CMAKE_CURRENT_LIST_DIR}/harness)
add_subdirectory(harness)

macro(add_test_suite suite_name)
  add_executable(${suite_name}_target
    ${suite_name}/${suite_name}.cpp
  )
  target_link_libraries(${suite_name}_target
    harness
    WasmVM
    cc
  )
  target_compile_definitions(${suite_name}_target PUBLIC CCTEST)
  set_target_properties(${suite_name}_target
    PROPERTIES OUTPUT_NAME ${suite_name}/${suite_name}
  )
  add_test(NAME ${suite_name}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${suite_name}/${suite_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${suite_name}
  )
  set_property(TEST ${suite_name} PROPERTY 
    ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/src/lib"
  )
endmacro(add_test_suite)

add_test_suite(SourceFile)
add_test_suite(pp_token)
add_test_suite(pp_directive)
add_test_suite(pp_macro)