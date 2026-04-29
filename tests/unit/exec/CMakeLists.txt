add_executable(wvmcc-preprocess-only-test preprocess_only_test.cpp)
add_dependencies(wvmcc-preprocess-only-test wvmcc)
add_test(NAME preprocess-only COMMAND wvmcc-preprocess-only-test)
