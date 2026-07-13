// Regression test: tokens that follow a collapsed construct (multiline block
// comment, line splice) must keep their physical source position, and a
// malformed literal must be diagnosed exactly once even when the token is
// both peeked and consumed.
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdio>

#include "pp/Tokenizer.hpp"
#include "pp/Preprocessor.hpp"

static int failures = 0;

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::cerr << "[FAIL] " << msg << "\n";                           \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

// A 4-line block comment collapses to one space; the tokens on line 5 must
// still report line 5 (the tokenizer used to lose the comment's newlines).
static void test_multiline_block_comment() {
    std::istringstream in(
        "/* line1\n"
        "line2\n"
        "line3\n"
        "line4 */\n"
        "char c = '\\777';\n");
    wvmcc::Tokenizer tz(in);
    while (auto t = tz.next()) {
        if (t->kind == wvmcc::PPTokenKind::CharConst) {
            CHECK(t->span.begin.line == 5,
                  "char constant after block comment: line "
                      << "expected 5, got " << t->span.begin.line);
            CHECK(t->span.begin.column == 10,
                  "char constant after block comment: column "
                      << "expected 10, got " << t->span.begin.column);
            return;
        }
    }
    CHECK(false, "char constant token not found after block comment");
}

// A line splice joins two physical lines into one logical line, but the token
// after the splice still lives on the second physical line.
static void test_line_splice() {
    std::istringstream in("int a\\\n= 1;\n");
    wvmcc::Tokenizer tz(in);
    while (auto t = tz.next()) {
        if (t->kind == wvmcc::PPTokenKind::Punctuator && t->lexeme == "=") {
            CHECK(t->span.begin.line == 2,
                  "token after line splice: line expected 2, got "
                      << t->span.begin.line);
            CHECK(t->span.begin.column == 1,
                  "token after line splice: column expected 1, got "
                      << t->span.begin.column);
            return;
        }
    }
    CHECK(false, "'=' token not found after line splice");
}

// An out-of-range octal escape must produce exactly one diagnostic even when
// the token is peeked before being consumed (peek + next used to each run
// normalization and emit the diagnostic again).
static void test_single_diagnostic_on_peek_then_next() {
    const std::string fname = "temp_comment_span.c";
    {
        std::ofstream ofs(fname);
        ofs << "/* line1\nline2\nline3\nline4 */\nchar c = '\\777';\n";
    }
    wvmcc::Preprocessor pp;
    if (!pp.open(fname)) {
        std::remove(fname.c_str());
        CHECK(false, "failed to open " << fname);
        return;
    }
    bool sawCharConst = false;
    while (true) {
        auto peeked = pp.peek();
        if (!peeked) break;
        auto t = pp.next();
        if (t && t->kind == wvmcc::PPTokenKind::CharConst) {
            sawCharConst = true;
            CHECK(t->span.begin.line == 5,
                  "preprocessed char constant: line expected 5, got "
                      << t->span.begin.line);
        }
    }
    int rangeDiags = 0;
    for (const auto& d : pp.getDiagnostics()) {
        if (d.message.find("octal escape sequence out of range") != std::string::npos) {
            ++rangeDiags;
            CHECK(d.span && d.span->begin.line == 5,
                  "octal-escape diagnostic: line expected 5, got "
                      << (d.span ? d.span->begin.line : -1));
        }
    }
    std::remove(fname.c_str());
    CHECK(sawCharConst, "char constant token not produced by preprocessor");
    CHECK(rangeDiags == 1,
          "octal-escape diagnostic count expected 1, got " << rangeDiags);
}

int main() {
    test_multiline_block_comment();
    test_line_splice();
    test_single_diagnostic_on_peek_then_next();
    if (failures) return 1;
    std::cout << "pp_comment_span_test: OK" << std::endl;
    return 0;
}
