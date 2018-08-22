include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_SOURCE_DIR}/include
)
enable_testing()
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/unittests)
function(add_unittest filepath)
    get_filename_component(unittest ${filepath} NAME_WE)
    get_filename_component(unittest_dir ${filepath} DIRECTORY)
    set(unittest_src "${PROJECT_BINARY_DIR}/test/unittests/${filepath}.cpp")
    configure_file(unittest.cpp.in ${unittest_src})
    add_executable(${unittest}
        ${unittest_src}
        ${filepath}.hpp
    )
    target_link_libraries(${unittest}
        wvmcc_base
        skypat
    )
    add_test(${unittest} ${PROJECT_BINARY_DIR}/test/unittests/${unittest})
endfunction(add_unittest unittest)

add_unittest(adt/Token_unittest)
add_unittest(Option_unittest)
add_unittest(ByteBuffer_unittest)
add_unittest(FileReader_unittest)
add_unittest(PassManager_unittest)