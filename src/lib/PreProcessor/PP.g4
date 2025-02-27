grammar PP;

options {
    language = Cpp;
}

import PPToken;

group : control_line | text_line;

control_line : define_obj
    | define_func
    | Hash pp_token* line_end;

text_line : pp_token* line_end;

define_obj : Hash WhiteSpace* 'define' WhiteSpace+ Identifier WhiteSpace+ pp_token* line_end;

define_func : Hash WhiteSpace* 'define' WhiteSpace+ Identifier '(' ( Ellipsis | identifier_list ( Comma WhiteSpace* Ellipsis )? )? WhiteSpace* ParenR pp_token* line_end;

identifier_list : (WhiteSpace* Identifier WhiteSpace* Comma)* WhiteSpace* Identifier WhiteSpace*;

line_end : NewLine? EOF | NewLine;

pp_token : HeaderName
    | Identifier
    | PPNumber
    | CharConst
    | StringLiteral
    | Punctuator
    | ParenL
    | ParenR
    | Hash
    | HashHash
    | Comma
    | Ellipsis
    | BlockComment
    | LineComment
    | WhiteSpace;
