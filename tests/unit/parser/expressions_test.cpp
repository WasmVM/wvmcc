// Unit test: parse expressions (assignments and conditional) and verify AST nodes
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <memory>
#include "pp/Preprocessor.hpp"
#include "parser/Lexer.hpp"
#include "parser/Parser.hpp"

using namespace wvmcc;
using namespace wvmcc::parser;

static int parse_return_expr(const std::string &src, ExprPtr &outExpr) {
    const std::string fname = "temp_expressions_test.c";
    {
        std::ofstream ofs(fname);
        ofs << src;
    }

    Preprocessor pp;
    if (!pp.open(fname)) { std::remove(fname.c_str()); return 2; }
    Lexer lex(pp);
    Parser parser(lex);
    auto tu = parser.parseTranslationUnit();
    if (!tu) { std::remove(fname.c_str()); return 3; }

    if (tu->externals.size() != 1) { std::remove(fname.c_str()); return 4; }
    auto ext = tu->externals[0];
    if (!std::holds_alternative<FunctionDefPtr>(ext->decl)) { std::remove(fname.c_str()); return 5; }
    auto f = std::get<FunctionDefPtr>(ext->decl);
    if (!f) { std::remove(fname.c_str()); return 6; }
    if (f->body.empty()) { std::remove(fname.c_str()); return 7; }

    auto bi = f->body[0];
    if (!std::holds_alternative<StmtPtr>(bi->item)) { std::remove(fname.c_str()); return 8; }
    auto st = std::get<StmtPtr>(bi->item);
    auto rs = std::dynamic_pointer_cast<ReturnStmt>(st);
    if (!rs) { std::remove(fname.c_str()); return 9; }

    if (rs->value.has_value()) outExpr = rs->value.value(); else outExpr = nullptr;
    std::remove(fname.c_str());
    return 0;
}

int main() {
    ExprPtr e;

    // simple assignment
    if (parse_return_expr("int main() { return x = 42; }\n", e) != 0) { std::cerr << "simple assignment parse failed" << std::endl; return 1; }
    auto be = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!be) { std::cerr << "expected BinaryExpr for simple assignment" << std::endl; return 2; }
    if (be->op != "=") { std::cerr << "expected '=' op, got '" << be->op << "'" << std::endl; return 3; }
    auto lhs_id = std::dynamic_pointer_cast<IdentifierExpr>(be->lhs);
    if (!lhs_id || lhs_id->name != "x") { std::cerr << "left-hand side is not identifier 'x'" << std::endl; return 4; }
    auto rhs_int = std::dynamic_pointer_cast<IntegerLiteral>(be->rhs);
    if (!rhs_int || rhs_int->value != 42) { std::cerr << "right-hand side is not integer 42" << std::endl; return 5; }

    // compound assignment
    if (parse_return_expr("int main() { return y += 3; }\n", e) != 0) { std::cerr << "compound assignment parse failed" << std::endl; return 6; }
    be = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!be) { std::cerr << "expected BinaryExpr for compound assignment" << std::endl; return 7; }
    if (be->op != "+=") { std::cerr << "expected '+=' op, got '" << be->op << "'" << std::endl; return 8; }

    // right-associative chain: a = b = 5
    if (parse_return_expr("int main() { return a = b = 5; }\n", e) != 0) { std::cerr << "chained assignment parse failed" << std::endl; return 9; }
    be = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!be) { std::cerr << "expected BinaryExpr for chained assignment" << std::endl; return 10; }
    if (be->op != "=") { std::cerr << "expected top '=' op, got '" << be->op << "'" << std::endl; return 11; }
    auto right_be = std::dynamic_pointer_cast<BinaryExpr>(be->rhs);
    if (!right_be) { std::cerr << "expected nested BinaryExpr on RHS for chained assignment" << std::endl; return 12; }
    if (right_be->op != "=") { std::cerr << "expected nested '=' op, got '" << right_be->op << "'" << std::endl; return 13; }

    // conditional operator: simple
    if (parse_return_expr("int main() { return x ? 1 : 2; }\n", e) != 0) { std::cerr << "simple conditional parse failed" << std::endl; return 14; }
    auto te = std::dynamic_pointer_cast<TernaryExpr>(e);
    if (!te) { std::cerr << "expected TernaryExpr for conditional" << std::endl; return 15; }
    auto cond_id = std::dynamic_pointer_cast<IdentifierExpr>(te->cond);
    if (!cond_id || cond_id->name != "x") { std::cerr << "conditional cond is not identifier 'x'" << std::endl; return 16; }
    auto then_int = std::dynamic_pointer_cast<IntegerLiteral>(te->thenExpr);
    auto else_int = std::dynamic_pointer_cast<IntegerLiteral>(te->elseExpr);
    if (!then_int || then_int->value != 1) { std::cerr << "then-expression is not integer 1" << std::endl; return 17; }
    if (!else_int || else_int->value != 2) { std::cerr << "else-expression is not integer 2" << std::endl; return 18; }

    // nested conditional: a ? b : c ? d : e  (else is nested ternary)
    if (parse_return_expr("int main() { return a ? b : c ? d : e; }\n", e) != 0) { std::cerr << "nested conditional parse failed" << std::endl; return 19; }
    te = std::dynamic_pointer_cast<TernaryExpr>(e);
    if (!te) { std::cerr << "expected TernaryExpr for top conditional" << std::endl; return 20; }
    auto nested = std::dynamic_pointer_cast<TernaryExpr>(te->elseExpr);
    if (!nested) { std::cerr << "expected nested TernaryExpr in else branch" << std::endl; return 21; }

    // primary: integer literal
    if (parse_return_expr("int main() { return 123; }\n", e) != 0) { std::cerr << "integer literal parse failed" << std::endl; return 80; }
    auto il = std::dynamic_pointer_cast<IntegerLiteral>(e);
    if (!il || il->value != 123) { std::cerr << "expected IntegerLiteral 123" << std::endl; return 81; }

    // primary: string literal
    if (parse_return_expr("int main() { return \"hello\"; }\n", e) != 0) { std::cerr << "string literal parse failed" << std::endl; return 82; }
    auto sl = std::dynamic_pointer_cast<StringLiteral>(e);
    if (!sl || sl->value.find("hello") == std::string::npos) { std::cerr << "expected StringLiteral 'hello'" << std::endl; return 83; }

    // primary: identifier
    if (parse_return_expr("int main() { return ident; }\n", e) != 0) { std::cerr << "identifier primary parse failed" << std::endl; return 84; }
    auto id = std::dynamic_pointer_cast<IdentifierExpr>(e);
    if (!id || id->name != "ident") { std::cerr << "expected IdentifierExpr 'ident'" << std::endl; return 85; }

    // generic selection: _Generic(controlling, type: expr, default: expr)
    if (parse_return_expr("int main() { return _Generic(x, int: 1, default: 2); }\n", e) != 0) { std::cerr << "_Generic parse failed" << std::endl; return 86; }
    auto ge = std::dynamic_pointer_cast<GenericSelectionExpr>(e);
    if (!ge) { std::cerr << "expected GenericSelectionExpr" << std::endl; return 87; }
    // controlling expression should be identifier 'x'
    auto ctrl = std::dynamic_pointer_cast<IdentifierExpr>(ge->controlling);
    if (!ctrl || ctrl->name != "x") { std::cerr << "expected controlling id 'x' in _Generic" << std::endl; return 88; }
    // expect at least two associations (int:1, default:2)
    if (ge->assocs.size() < 2) { std::cerr << "expected associations in _Generic" << std::endl; return 89; }
    // last association should be default with integer 2
    auto &last = ge->assocs.back();
    if (!last.isDefault) { std::cerr << "expected last assoc to be default" << std::endl; return 90; }
    auto defval = std::dynamic_pointer_cast<IntegerLiteral>(last.expr);
    if (!defval || defval->value != 2) { std::cerr << "expected default expr integer 2" << std::endl; return 91; }

        // Precedence / associativity combinational tests
        // multiplicative binds tighter than additive: a + b * c => a + (b * c)
        if (parse_return_expr("int main() { return a + b * c; }\n", e) != 0) { std::cerr << "precedence a + b * c parse failed" << std::endl; return 92; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != "+") { std::cerr << "expected top '+' for a + b * c" << std::endl; return 93; }
            auto rhs = std::dynamic_pointer_cast<BinaryExpr>(top->rhs);
            if (!rhs || rhs->op != "*") { std::cerr << "expected rhs '*' for a + b * c" << std::endl; return 94; }
        }

        // additive binds tighter than shift: a << b + c => a << (b + c)
        if (parse_return_expr("int main() { return a << b + c; }\n", e) != 0) { std::cerr << "precedence a << b + c parse failed" << std::endl; return 95; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != "<<") { std::cerr << "expected top '<<' for a << b + c" << std::endl; return 96; }
            auto rhs = std::dynamic_pointer_cast<BinaryExpr>(top->rhs);
            if (!rhs || rhs->op != "+") { std::cerr << "expected rhs '+' for a << b + c" << std::endl; return 97; }
        }

        // bitwise & binds tighter than |: a | b & c => a | (b & c)
        if (parse_return_expr("int main() { return a | b & c; }\n", e) != 0) { std::cerr << "precedence a | b & c parse failed" << std::endl; return 98; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != "|") { std::cerr << "expected top '|' for a | b & c" << std::endl; return 99; }
            auto rhs = std::dynamic_pointer_cast<BinaryExpr>(top->rhs);
            if (!rhs || rhs->op != "&") { std::cerr << "expected rhs '&' for a | b & c" << std::endl; return 100; }
        }

        // logical OR lower precedence than AND: a || b && c => a || (b && c)
        if (parse_return_expr("int main() { return a || b && c; }\n", e) != 0) { std::cerr << "precedence a || b && c parse failed" << std::endl; return 101; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != "||") { std::cerr << "expected top '||' for a || b && c" << std::endl; return 102; }
            auto rhs = std::dynamic_pointer_cast<BinaryExpr>(top->rhs);
            if (!rhs || rhs->op != "&&") { std::cerr << "expected rhs '&&' for a || b && c" << std::endl; return 103; }
        }

        // left-associative check: a - b - c => (a - b) - c
        if (parse_return_expr("int main() { return a - b - c; }\n", e) != 0) { std::cerr << "associativity a - b - c parse failed" << std::endl; return 104; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != "-") { std::cerr << "expected top '-' for a - b - c" << std::endl; return 105; }
            auto left = std::dynamic_pointer_cast<BinaryExpr>(top->lhs);
            if (!left || left->op != "-") { std::cerr << "expected left child '-' for a - b - c (left-assoc)" << std::endl; return 106; }
        }

        // comma operator is left-associative: a , b , c => (a,b),c
        if (parse_return_expr("int main() { return a , b , c; }\n", e) != 0) { std::cerr << "comma associativity parse failed" << std::endl; return 107; }
        {
            auto top = std::dynamic_pointer_cast<BinaryExpr>(e);
            if (!top || top->op != ",") { std::cerr << "expected top ',' for a , b , c" << std::endl; return 108; }
            auto left = std::dynamic_pointer_cast<BinaryExpr>(top->lhs);
            if (!left || left->op != ",") { std::cerr << "expected left child ',' for a , b , c (left-assoc)" << std::endl; return 109; }
        }
    // logical OR
    if (parse_return_expr("int main() { return a || b; }\n", e) != 0) { std::cerr << "logical OR parse failed" << std::endl; return 22; }
    auto bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "||") { std::cerr << "expected '||' BinaryExpr" << std::endl; return 23; }

    // logical AND
    if (parse_return_expr("int main() { return a && b; }\n", e) != 0) { std::cerr << "logical AND parse failed" << std::endl; return 24; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "&&") { std::cerr << "expected '&&' BinaryExpr" << std::endl; return 25; }

    // bitwise inclusive OR
    if (parse_return_expr("int main() { return a | b; }\n", e) != 0) { std::cerr << "bitwise OR parse failed" << std::endl; return 26; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "|") { std::cerr << "expected '|' BinaryExpr" << std::endl; return 27; }

    // bitwise exclusive OR
    if (parse_return_expr("int main() { return a ^ b; }\n", e) != 0) { std::cerr << "bitwise XOR parse failed" << std::endl; return 28; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "^") { std::cerr << "expected '^' BinaryExpr" << std::endl; return 29; }

    // bitwise AND
    if (parse_return_expr("int main() { return a & b; }\n", e) != 0) { std::cerr << "bitwise AND parse failed" << std::endl; return 30; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "&") { std::cerr << "expected '&' BinaryExpr" << std::endl; return 31; }

    // equality ==
    if (parse_return_expr("int main() { return a == b; }\n", e) != 0) { std::cerr << "equality == parse failed" << std::endl; return 32; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "==") { std::cerr << "expected '==' BinaryExpr" << std::endl; return 33; }

    // inequality !=
    if (parse_return_expr("int main() { return a != b; }\n", e) != 0) { std::cerr << "inequality != parse failed" << std::endl; return 34; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "!=") { std::cerr << "expected '!=' BinaryExpr" << std::endl; return 35; }

    // relational < > <= >=
    if (parse_return_expr("int main() { return a < b; }\n", e) != 0) { std::cerr << "relational '<' parse failed" << std::endl; return 36; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "<") { std::cerr << "expected '<' BinaryExpr" << std::endl; return 37; }

    if (parse_return_expr("int main() { return a > b; }\n", e) != 0) { std::cerr << "relational '>' parse failed" << std::endl; return 38; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != ">") { std::cerr << "expected '>' BinaryExpr" << std::endl; return 39; }

    if (parse_return_expr("int main() { return a <= b; }\n", e) != 0) { std::cerr << "relational '<=' parse failed" << std::endl; return 40; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "<=") { std::cerr << "expected '<=' BinaryExpr" << std::endl; return 41; }

    if (parse_return_expr("int main() { return a >= b; }\n", e) != 0) { std::cerr << "relational '>=' parse failed" << std::endl; return 42; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != ">=") { std::cerr << "expected '>=' BinaryExpr" << std::endl; return 43; }

    // shift << >>
    if (parse_return_expr("int main() { return a << b; }\n", e) != 0) { std::cerr << "shift '<<' parse failed" << std::endl; return 44; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "<<") { std::cerr << "expected '<<' BinaryExpr" << std::endl; return 45; }

    if (parse_return_expr("int main() { return a >> b; }\n", e) != 0) { std::cerr << "shift '>>' parse failed" << std::endl; return 46; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != ">>") { std::cerr << "expected '>>' BinaryExpr" << std::endl; return 47; }

    // multiplicative * / %
    if (parse_return_expr("int main() { return a * b; }\n", e) != 0) { std::cerr << "multiplicative '*' parse failed" << std::endl; return 48; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "*") { std::cerr << "expected '*' BinaryExpr" << std::endl; return 49; }

    if (parse_return_expr("int main() { return a / b; }\n", e) != 0) { std::cerr << "multiplicative '/' parse failed" << std::endl; return 50; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "/") { std::cerr << "expected '/' BinaryExpr" << std::endl; return 51; }

    if (parse_return_expr("int main() { return a % b; }\n", e) != 0) { std::cerr << "multiplicative '%' parse failed" << std::endl; return 52; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "%") { std::cerr << "expected '%' BinaryExpr" << std::endl; return 53; }

    // additive + -
    if (parse_return_expr("int main() { return a + b; }\n", e) != 0) { std::cerr << "additive '+' parse failed" << std::endl; return 54; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "+") { std::cerr << "expected '+' BinaryExpr" << std::endl; if (bo) std::cerr << "got op '" << bo->op << "'" << std::endl; return 55; }

    if (parse_return_expr("int main() { return a - b; }\n", e) != 0) { std::cerr << "additive '-' parse failed" << std::endl; return 56; }
    bo = std::dynamic_pointer_cast<BinaryExpr>(e);
    if (!bo || bo->op != "-") { std::cerr << "expected '-' BinaryExpr" << std::endl; return 57; }

    // compound literal: (int){42} (C17 §6.5.2.5)
    if (parse_return_expr("int main() { return (int){42}; }\n", e) != 0) { std::cerr << "scalar compound literal parse failed" << std::endl; return 110; }
    {
        auto cl = std::dynamic_pointer_cast<CompoundLiteral>(e);
        if (!cl) { std::cerr << "expected CompoundLiteral for (int){42}" << std::endl; return 111; }
    }

    // compound literal with multiple elements
    if (parse_return_expr("int main() { return (double){3.14}; }\n", e) != 0) { std::cerr << "double compound literal parse failed" << std::endl; return 112; }
    {
        auto cl = std::dynamic_pointer_cast<CompoundLiteral>(e);
        if (!cl) { std::cerr << "expected CompoundLiteral for (double){3.14}" << std::endl; return 113; }
    }

    std::cout << "expressions-test: OK" << std::endl;
    return 0;
}
