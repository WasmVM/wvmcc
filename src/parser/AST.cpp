#include "AST.hpp"

namespace wvmcc::parser {

std::vector<std::string> DeclarationSpecifiers::to_vector() const {
    std::vector<std::string> out;
    if (hasStorage(StorageClass::Typedef)) out.push_back("typedef");
    if (hasStorage(StorageClass::Extern)) out.push_back("extern");
    if (hasStorage(StorageClass::Static)) out.push_back("static");
    if (hasStorage(StorageClass::Auto)) out.push_back("auto");
    if (hasStorage(StorageClass::Register)) out.push_back("register");
    if (hasStorage(StorageClass::ThreadLocal)) out.push_back("_Thread_local");
    for (auto &t : typeSpec) out.push_back(t);
    if (hasTypeQual(TypeQualifier::Const)) out.push_back("const");
    if (hasTypeQual(TypeQualifier::Volatile)) out.push_back("volatile");
    if (hasTypeQual(TypeQualifier::Restrict)) out.push_back("restrict");
    if (hasTypeQual(TypeQualifier::Atomic)) out.push_back("_Atomic");
    if (hasFuncSpec(FunctionSpecifier::Inline)) out.push_back("inline");
    if (hasFuncSpec(FunctionSpecifier::NoReturn)) out.push_back("_Noreturn");
    for (auto &a : alignSpec) out.push_back(a);
    return out;
}

} // namespace wvmcc::parser
