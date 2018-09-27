add_library(wvmcc-parsergen-objs OBJECT
    "RuleParser.cpp"
    "Rule.cpp"
    "FirstFollow.cpp"
    "FirstFollowExtractor.cpp"
)
add_executable(wvmcc-parsergen
    "main.cpp"
    $<TARGET_OBJECTS:wvmcc-parsergen-objs>
)
target_link_libraries(wvmcc-parsergen
    wvmcc_base
)
install(TARGETS wvmcc-parsergen
    RUNTIME DESTINATION bin
)