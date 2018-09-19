include_directories(${CMAKE_CURRENT_LIST_DIR})
add_executable(wvmcc-parsergen
    "main.cpp"
    "RuleParser.cpp"
)
target_link_libraries(wvmcc-parsergen
    wvmcc_base
)
install(TARGETS wvmcc-parsergen
    RUNTIME DESTINATION bin
)