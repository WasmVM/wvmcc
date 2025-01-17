grammar PP;

options {
    language = Cpp;
}

import PPToken;

group : text_line | EOF;

text_line : pp_token* NewLine;

pp_token : HeaderName | Identifier | PPNumber | CharConst | StringLiteral | Punctuator | BlockComment | LineComment | WhiteSpace;