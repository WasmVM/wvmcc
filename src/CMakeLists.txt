include_directories(include)
add_subdirectory(lib)
add_subdirectory(exec)
install(DIRECTORY include/
    DESTINATION include/wvmcc
    FILES_MATCHING PATTERN "*.h"
)