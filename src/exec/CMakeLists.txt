add_executable(wvmcc
    "wvmcc.c"
)
target_link_libraries(wvmcc
    wvmcc_base
)
install(
    TARGETS wvmcc
    RUNTIME DESTINATION bin/
)