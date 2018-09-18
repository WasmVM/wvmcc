include_directories(include)
add_subdirectory(lib)
add_subdirectory(exec)
add_subdirectory(tools)

install(DIRECTORY include/
    DESTINATION include/wvmcc
    FILES_MATCHING PATTERN "*.h"
)