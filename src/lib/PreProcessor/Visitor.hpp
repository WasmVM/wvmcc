#ifndef WVMCC_PreProcessor_Visitor_DEF
#define WVMCC_PreProcessor_Visitor_DEF

#include <PreProcessor.hpp>
#include <PPVisitor.h>
#include <PPLexer.h>
#include <optional>
#include <memory>
#include <fstream>

namespace WasmVM {

struct PreProcessor::Visitor : public PPVisitor {

    Visitor(std::filesystem::path path, PreProcessor& pp);
    bool fetch(); // false if match EOF

    std::filesystem::path path;

protected:
    virtual std::any visitGroup(PPParser::GroupContext *) override;
    virtual std::any visitText_line(PPParser::Text_lineContext *) override;
    virtual std::any visitPp_token(PPParser::Pp_tokenContext *ctx) override;
    virtual std::any visitLine_end(PPParser::Line_endContext* ctx) override;
    virtual std::any visitControl_line(PPParser::Control_lineContext* ctx) override;
    virtual std::any visitDefine_obj(PPParser::Define_objContext* ctx) override;
    virtual std::any visitDefine_func(PPParser::Define_funcContext *ctx) override;
    virtual std::any visitIdentifier_list(PPParser::Identifier_listContext *ctx) override;

    std::ifstream fin;
    std::unique_ptr<antlr4::ANTLRInputStream> input;
    std::unique_ptr<PPLexer> lexer;
    std::unique_ptr<antlr4::CommonTokenStream> tokens;
    std::unique_ptr<PPParser> parser;
    PreProcessor& pp;
};

} // namespace WasmVM

#endif