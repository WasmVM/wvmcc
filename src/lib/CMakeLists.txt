add_library(wvmcc_base
    "adt/List.c"
    "adt/Map.c"
    "adt/Token.c"
    "adt/Vector.c"
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
    "lexer/string.c"
    "lexer/punctuator.c"
)

install(TARGETS wvmcc_base
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)