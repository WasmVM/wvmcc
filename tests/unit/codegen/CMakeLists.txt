# CMake file for codegen unit tests

add_executable(basic_codegen_test
    basic_codegen_test.cpp
)

add_executable(expression_emission_test
    expression_emission_test.cpp
)

add_executable(statement_emission_test
    statement_emission_test.cpp
)

target_link_libraries(basic_codegen_test
    wvmcc
)

target_link_libraries(expression_emission_test
    wvmcc
)

target_link_libraries(statement_emission_test
    wvmcc
)

add_test(NAME basic_codegen_test COMMAND basic_codegen_test)
add_test(NAME expression_emission_test COMMAND expression_emission_test)
add_test(NAME statement_emission_test COMMAND statement_emission_test)
