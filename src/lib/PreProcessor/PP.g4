grammar PP;

options {
    language = Cpp;
}

import PPToken;

group : control_line | text_line;

control_line : define_obj
    | Hash pp_token* line_end;

text_line : pp_token* line_end;

define_obj : Hash WhiteSpace* 'define' WhiteSpace+ Identifier WhiteSpace+ pp_token* line_end;

line_end : NewLine? EOF | NewLine;

pp_token : HeaderName | Identifier | PPNumber | CharConst | StringLiteral | Punctuator | BlockComment | LineComment | WhiteSpace;
