add_library(wvmcc_base
    "adt/List.c"
    "adt/Map.c"
    "adt/Token.c"
    "Option.c"
    "Util.c"
    "PassManager.c"
    "ByteBuffer.c"
    "FileReader.c"
    "TokenBuffer.c"
    "lexer/Lexer.c"
    "lexer/identifier.c"
    "lexer/keyword.c"
    "lexer/integer.c"
    "lexer/floating.c"
    "lexer/character.c"
)

install(TARGETS wvmcc_base
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)