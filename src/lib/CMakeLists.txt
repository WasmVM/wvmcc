add_library(wvmcc_base
    "adt/list.c"
    "adt/map.c"
    "adt/Token.c"
    "Option.c"
    "PassManager.c"
    "ByteBuffer.c"
    "FileReader.c"
    "TokenBuffer.c"
)

install(TARGETS wvmcc_base
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)