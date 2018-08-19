include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_SOURCE_DIR}/include
)
message(${PROJECT_SOURCE_DIR})
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test/unittests)
function(add_unittest unittest)
    set(unittest_src "${PROJECT_BINARY_DIR}/test/unittests/${unittest}.cpp")
    configure_file(unittest.cpp.in ${unittest_src})
    add_executable(${unittest}
        ${unittest_src}
        ${unittest}.hpp
    )
    target_link_libraries(${unittest}
        wvmcc_base
        ${SKYPAT_LIBRARIES}
    )
    install(TARGETS ${unittest}
        RUNTIME DESTINATION libexec/wvmcc
    )
endfunction(add_unittest unittest)

add_unittest(Option_unittest)
add_unittest(ByteBuffer_unittest)
add_unittest(FileReader_unittest)