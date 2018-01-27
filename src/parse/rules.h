#ifndef WVMCC_PARSE_RULES_DEF
#define WVMCC_PARSE_RULES_DEF

#include "token.h"

// Return -1 if error, 0 if success
int startParse(FileInst* fInst, FILE* fout);

// Return 1 if match, 0 if not match, but not indicate whether error
// happened or not

// External definitions
int translation_unit(FileInst* fInst);
int external_declaration(FileInst* fInst);
int function_definition(FileInst* fInst);
int declaration_list(FileInst* fInst);

// Declarations
int declaration(FileInst* fInst);
int declaration_specifiers(FileInst* fInst);
int init_declarator_list(FileInst* fInst);
int init_declarator(FileInst* fInst);
int storage_class_specifier(FileInst* fInst);
int type_specifier(FileInst* fInst);
int struct_or_union_specifier(FileInst* fInst);
int struct_or_union(FileInst* fInst);
int struct_declaration_list(FileInst* fInst);
int struct_declaration(FileInst* fInst);
int specifier_qualifier_list(FileInst* fInst);
int struct_declarator_list(FileInst* fInst);
int struct_declarator(FileInst* fInst);
int enum_specifier(FileInst* fInst);
int enumerator_list(FileInst* fInst);
int enumerator(FileInst* fInst);
int atomic_type_specifier(FileInst* fInst);
int type_qualifier(FileInst* fInst);
int function_specifier(FileInst* fInst);
int alignment_specifier(FileInst* fInst);
int declarator(FileInst* fInst);
int direct_declarator(FileInst* fInst);
int pointer(FileInst* fInst);
int type_qualifier_list(FileInst* fInst);
int parameter_type_list(FileInst* fInst);
int parameter_list(FileInst* fInst);
int parameter_declaration(FileInst* fInst);
int identifier_list(FileInst* fInst);
int type_name(FileInst* fInst);
int abstract_declarator(FileInst* fInst);
int direct_abstract_declarator(FileInst* fInst);
int typedef_name(FileInst* fInst);
int initializer(FileInst* fInst);
int initializer_list(FileInst* fInst);
int designation(FileInst* fInst);
int designator_list(FileInst* fInst);
int designtor(FileInst* fInst);
int static_assert_declaration(FileInst* fInst);

// Expressions
int primary_expression(FileInst* fInst);
int generic_selection(FileInst* fInst);
int generic_assoc_list(FileInst* fInst);
int generic_association(FileInst* fInst);
int postfix_expression(FileInst* fInst);
int argument_expression_list(FileInst* fInst);
int unary_expression(FileInst* fInst);
int unary_operator(FileInst* fInst);
int cast_expression(FileInst* fInst);
int multiplicative_expression(FileInst* fInst);
int additive_expression(FileInst* fInst);
int shift_expression(FileInst* fInst);
int relational_expression(FileInst* fInst);
int equality_expression(FileInst* fInst);
int and_expression(FileInst* fInst);
int exclusive_or_expression(FileInst* fInst);
int inclusive_or_expression(FileInst* fInst);
int logical_and_expression(FileInst* fInst);
int logical_or_expression(FileInst* fInst);
int conditional_expression(FileInst* fInst);
int assignment_expression(FileInst* fInst);
int assignment_operator(FileInst* fInst);
int expression(FileInst* fInst);
int constant_expression(FileInst* fInst);

// Statement
int statement(FileInst* fInst);
int labeled_statement(FileInst* fInst);
int compound_statement(FileInst* fInst);
int block_item_list(FileInst* fInst);
int block_item(FileInst* fInst);
int expression_statement(FileInst* fInst);
int selection_statement(FileInst* fInst);
int iteration_statement(FileInst* fInst);
int jump_statement(FileInst* fInst);
#endif