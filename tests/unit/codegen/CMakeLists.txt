# CMake file for codegen unit tests

# Find GTest
find_package(GTest REQUIRED)

# Create the test executable
add_executable(basic_codegen_test 
    basic_codegen_test.cpp
)

# Link with GTest and wvmcc
target_link_libraries(basic_codegen_test 
    GTest::gtest 
    GTest::gtest_main
    wvmcc
)

# Add the test to the test suite
add_test(NAME basic_codegen_test COMMAND basic_codegen_test)