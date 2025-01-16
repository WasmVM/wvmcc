lexer grammar PPToken;

options {
    language = Cpp;
}

/* Tri Graph */
fragment Hash_ : '??=' | '#';
fragment Bar_ : '??!' | '|';
fragment BSlash_ : '??/' | '\\';
fragment Caret_ : '??\'' | '^';
fragment Tlide_ : '??-' | '~';
fragment BracketL_ : '??(' | '[';
fragment BracketR_ : '??)' | ']';
fragment BraceL_ : '??<' | '{';
fragment BraceR_ : '??>' | '}';

/* Line concat */
LineConcat : BSlash_ '\n' -> skip;

/* Header name */
HeaderName : '<' ('/'~[/*] | ~('\\' | '\n' | '>') | Caret_ | Tlide_ | BracketL_ | BracketR_ | BraceL_ | BraceR_ | Bar_ | Hash_)+ '>'
    | '"' ('/'~[/*] | ~('\\' | '\n' | '"') | Caret_ | Tlide_ | BracketL_ | BracketR_ | BraceL_ | BraceR_ | Bar_ | Hash_)+ '"'
;

/* Identifier */
Identifier : ([_a-zA-Z] | UniCharName) ([_a-zA-Z0-9] | UniCharName)*;

/* PP Number */
PPNumber : '.'? [0-9] ([_a-zA-Z0-9.] | UniCharName | [eEpP] [+\-]?)*;

/* Character constant */
CharConst : [LuU]? '\'' (Escape | ~['\\\n] | Caret_ | Tlide_ | BracketL_ | BracketR_ | BraceL_ | BraceR_ | Bar_ | Hash_) '\'';
Escape : BSlash_ ([abfnrtv'"?\\] | BSlash_ | [0-7] [0-7]? [0-7]? | 'x' HexDigit HexDigit+) | UniCharName;

/* String literal */
StringLiteral : ([LuU] | 'u8')? '"' (~["\\\n] | Escape)* '"';

/* Punctuator */
Punctuator : '%:%:' | '<<=' | '>>=' | '...' | '%:' | '++' | '<'[:%<]? | [:%>]?'>' | ([*/%+\-&^|><!=] | Bar_ | Caret_)?'=' | '&&' | '##' | Hash_ Hash_ | '-' [\->] | BraceL_ | BraceR_ | BracketL_ | BracketR_ | (Bar_ Bar_?) | Caret_ | Tlide_ | Hash_ | [().&*+\-!/%?:;,];

/* White space */
WhiteSpace : [\t\f\r \u000B]+ ;
NewLine : [\n] ;

/* Universal Character Name */
UniCharName : BSlash_ 'u' HexQuad | BSlash_ 'U' HexQuad HexQuad;
HexQuad : HexDigit HexDigit HexDigit HexDigit;
fragment HexDigit : [0-9a-fA-F];

/* Comment */
BlockComment : '/*' .*? '*/' ;
LineComment : '//' ~[\n]* ;
