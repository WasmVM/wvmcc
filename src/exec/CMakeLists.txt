include_directories(${CMAKE_CURRENT_LIST_DIR})
add_executable(wvmcc
    "wvmcc.c"
    "CCOption.c"
)
target_link_libraries(wvmcc
    wvmcc_base
)
install(
    TARGETS wvmcc
    RUNTIME DESTINATION bin/
)