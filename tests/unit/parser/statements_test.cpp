// Unit test: parse statements (if/switch/loops/jump/labels) and verify AST nodes
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

static int parse_first_function(const std::string &src, FunctionDefPtr &outF, std::vector<wvmcc::Diagnostic> &outDiags) {
    const std::string fname = "temp_statements_test.c";
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
    outF = f;
    outDiags = parser.getDiagnostics();
    std::remove(fname.c_str());
    return 0;
}

int main() {
    FunctionDefPtr f;
    std::vector<wvmcc::Diagnostic> diags;

    // if-else
    {
        std::string src = "int main() { if (x) y = 1; else y = 2; return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "if-else parse failed" << std::endl; return 1; }
        if (f->body.empty()) { std::cerr << "if-else: empty body" << std::endl; return 2; }
        auto bi = f->body[0];
        if (!std::holds_alternative<StmtPtr>(bi->item)) { std::cerr << "if-else: expected statement" << std::endl; return 3; }
        auto st = std::get<StmtPtr>(bi->item);
        auto ifs = std::dynamic_pointer_cast<IfStmt>(st);
        if (!ifs) { std::cerr << "if-else: expected IfStmt" << std::endl; return 4; }
    }

    // while loop
    {
        std::string src = "int main() { while (i) { i = i - 1; } return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "while parse failed" << std::endl; return 5; }
        auto bi = f->body[0];
        auto st = std::get<StmtPtr>(bi->item);
        auto ws = std::dynamic_pointer_cast<WhileStmt>(st);
        if (!ws) { std::cerr << "expected WhileStmt" << std::endl; return 6; }
    }

    // do-while loop
    {
        std::string src = "int main() { do { x = x + 1; } while (x < 10); return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "do-while parse failed" << std::endl; return 7; }
        auto bi = f->body[0];
        auto st = std::get<StmtPtr>(bi->item);
        auto ds = std::dynamic_pointer_cast<DoWhileStmt>(st);
        if (!ds) { std::cerr << "expected DoWhileStmt" << std::endl; return 8; }
    }

    // for loop with expression init
    {
        std::string src = "int main() { for (i = 0; i < 10; ++i) x = i; return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "for(expr) parse failed" << std::endl; return 9; }
        auto bi = f->body[0];
        auto st = std::get<StmtPtr>(bi->item);
        auto fs = std::dynamic_pointer_cast<ForStmt>(st);
        if (!fs) { std::cerr << "expected ForStmt (expr init)" << std::endl; return 10; }
        if (!fs->init.has_value()) { std::cerr << "for: expected init expression" << std::endl; return 11; }
    }

    // for loop with declaration init
    {
        std::string src = "int main() { for (int i = 0; i < 5; ++i) ; return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "for(decl) parse failed" << std::endl; return 12; }
        auto bi = f->body[0];
        auto st = std::get<StmtPtr>(bi->item);
        auto fs = std::dynamic_pointer_cast<ForStmt>(st);
        if (!fs) { std::cerr << "expected ForStmt (decl init)" << std::endl; return 13; }
        if (!fs->init.has_value()) { std::cerr << "for(decl): expected init declaration" << std::endl; return 14; }
    }

    // switch with case/default
    {
        std::string src = "int main() { switch(x) { case 1: a = 1; break; default: a = 2; } return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "switch parse failed" << std::endl; return 15; }
        auto bi = f->body[0];
        auto st = std::get<StmtPtr>(bi->item);
        auto ss = std::dynamic_pointer_cast<SwitchStmt>(st);
        if (!ss) { std::cerr << "expected SwitchStmt" << std::endl; return 16; }
        auto body = std::dynamic_pointer_cast<CompoundStmt>(ss->body);
        if (!body) { std::cerr << "switch body not compound" << std::endl; return 17; }
        bool sawCase = false, sawDefault = false;
        for (auto &it : body->items) {
            if (std::holds_alternative<StmtPtr>(it->item)) {
                auto s = std::get<StmtPtr>(it->item);
                if (std::dynamic_pointer_cast<CaseStmt>(s)) sawCase = true;
                if (std::dynamic_pointer_cast<DefaultStmt>(s)) sawDefault = true;
            }
        }
        if (!sawCase || !sawDefault) { std::cerr << "expected case and default in switch body" << std::endl; return 18; }
    }

    // labels and goto
    {
        std::string src = "int main() { goto L; x = 1; L: x = 2; return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "goto/label parse failed" << std::endl; return 19; }
        bool sawGoto = false, sawLabel = false;
        for (auto &it : f->body) {
            if (std::holds_alternative<StmtPtr>(it->item)) {
                auto s = std::get<StmtPtr>(it->item);
                if (std::dynamic_pointer_cast<GotoStmt>(s)) sawGoto = true;
                if (std::dynamic_pointer_cast<LabelStmt>(s)) sawLabel = true;
            }
        }
        if (!sawGoto || !sawLabel) { std::cerr << "expected goto and label" << std::endl; return 20; }
    }

    // break/continue placement: ensure diagnostics emitted when outside loops (parser produces diagnostics)
    {
        std::string src = "int main() { break; continue; return 0; }\n";
        if (parse_first_function(src, f, diags) != 0) { std::cerr << "break/continue parse failed" << std::endl; return 21; }
        if (diags.empty()) { std::cerr << "expected diagnostics for break/continue outside loop" << std::endl; return 22; }
    }

    std::cout << "statements-test: OK" << std::endl;
    return 0;
}
