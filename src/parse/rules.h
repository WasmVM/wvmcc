#ifndef WVMCC_PARSE_RULES_DEF
#define WVMCC_PARSE_RULES_DEF

#include "structs.h"
#include "token.h"

// Return -1 if error, 0 if success
int startParse(FileInst* fInst, FILE* fout);  // TODO:

// Return 1 if match, 0 if not match, but not indicate whether error
// happened or not

// External definitions
int translation_unit(FileInst* fInst, Map* typeMap);      // TODO:
int external_declaration(FileInst* fInst, Map* typeMap);  // TODO:
int function_definition(FileInst* fInst, Map* typeMap);   // TODO:
int declaration_list(FileInst* fInst, Map* typeMap);      // TODO:
int preprocessor_hint(FileInst* fInst);

// Declarations
int declaration(FileInst* fInst, Map* typeMap);  // TODO:
int declaration_specifiers(FileInst* fInst,
                           Map* typeMap,
                           Type** declTypePtr);              // TODO:
int init_declarator_list(FileInst* fInst, Map* typeMap);  // TODO:
int init_declarator(FileInst* fInst, Map* typeMap);       // TODO:
int storage_class_specifier(FileInst* fInst, Map* typeMap, Type* declType);
int type_specifier(FileInst* fInst, Map* typeMap, Type** declTypePtr);  // TODO:
int struct_or_union_specifier(FileInst* fInst,
                              Map* typeMap,
                              Type** declTypePtr);       // TODO:
int struct_or_union(FileInst* fInst, Map* typeMap);  // TODO:
int struct_declaration_list(FileInst* fInst,
                            Map* typeMap,
                            Type** declTypePtr);                            // TODO:
int struct_declaration(FileInst* fInst, Map* typeMap, Type** declTypePtr);  // TODO:
int specifier_qualifier_list(FileInst* fInst,
                             Map* typeMap,
                             Type** declTypePtr);  // TODO:
int struct_declarator_list(FileInst* fInst,
                           Map* typeMap,
                           Type** declTypePtr);                            // TODO:
int struct_declarator(FileInst* fInst, Map* typeMap, Type** declTypePtr);  // TODO:
int enum_specifier(FileInst* fInst, Map* typeMap);                     // TODO:
int enumerator_list(FileInst* fInst, Map* typeMap);                    // TODO:
int enumerator(FileInst* fInst, Map* typeMap);                         // TODO:
int atomic_type_specifier(FileInst* fInst, Map* typeMap);              // TODO:
int type_qualifier(FileInst* fInst, Map* typeMap);                     // TODO:
int function_specifier(FileInst* fInst, Map* typeMap);                 // TODO:
int alignment_specifier(FileInst* fInst, Map* typeMap);                // TODO:
int declarator(FileInst* fInst, Map* typeMap);                         // TODO:
int direct_declarator(FileInst* fInst, Map* typeMap);                  // TODO:
int pointer(FileInst* fInst, Map* typeMap);                            // TODO:
int type_qualifier_list(FileInst* fInst, Map* typeMap);                // TODO:
int parameter_type_list(FileInst* fInst,
                        Map* typeMap,
                        Type** declTypePtr);                            // TODO:
int parameter_list(FileInst* fInst, Map* typeMap, Type** declTypePtr);  // TODO:
int parameter_declaration(FileInst* fInst,
                          Map* typeMap,
                          Type** declTypePtr);           // TODO:
int identifier_list(FileInst* fInst, Map* typeMap);  // TODO:
int type_name(FileInst* fInst, Map* typeMap);        // TODO:
int abstract_declarator(FileInst* fInst,
                        Map* typeMap,
                        Type** declTypePtr);  // TODO:
int direct_abstract_declarator(FileInst* fInst,
                               Map* typeMap,
                               Type** declTypePtr);                // TODO:
int typedef_name(FileInst* fInst, Map* typeMap);               // TODO:
int initializer(FileInst* fInst, Map* typeMap);                // TODO:
int initializer_list(FileInst* fInst, Map* typeMap);           // TODO:
int designation(FileInst* fInst, Map* typeMap);                // TODO:
int designator_list(FileInst* fInst, Map* typeMap);            // TODO:
int designtor(FileInst* fInst, Map* typeMap);                  // TODO:
int static_assert_declaration(FileInst* fInst, Map* typeMap);  // TODO:

// Expressions
int primary_expression(FileInst* fInst, Map* typeMap);         // TODO:
int generic_selection(FileInst* fInst, Map* typeMap);          // TODO:
int generic_assoc_list(FileInst* fInst, Map* typeMap);         // TODO:
int generic_association(FileInst* fInst, Map* typeMap);        // TODO:
int postfix_expression(FileInst* fInst, Map* typeMap);         // TODO:
int argument_expression_list(FileInst* fInst, Map* typeMap);   // TODO:
int unary_expression(FileInst* fInst, Map* typeMap);           // TODO:
int unary_operator(FileInst* fInst, Map* typeMap);             // TODO:
int cast_expression(FileInst* fInst, Map* typeMap);            // TODO:
int multiplicative_expression(FileInst* fInst, Map* typeMap);  // TODO:
int additive_expression(FileInst* fInst, Map* typeMap);        // TODO:
int shift_expression(FileInst* fInst, Map* typeMap);           // TODO:
int relational_expression(FileInst* fInst, Map* typeMap);      // TODO:
int equality_expression(FileInst* fInst, Map* typeMap);        // TODO:
int and_expression(FileInst* fInst, Map* typeMap);             // TODO:
int exclusive_or_expression(FileInst* fInst, Map* typeMap);    // TODO:
int inclusive_or_expression(FileInst* fInst, Map* typeMap);    // TODO:
int logical_and_expression(FileInst* fInst, Map* typeMap);     // TODO:
int logical_or_expression(FileInst* fInst, Map* typeMap);      // TODO:
int conditional_expression(FileInst* fInst, Map* typeMap);     // TODO:
int assignment_expression(FileInst* fInst, Map* typeMap);      // TODO:
int assignment_operator(FileInst* fInst, Map* typeMap);        // TODO:
int expression(FileInst* fInst, Map* typeMap);                 // TODO:
int constant_expression(FileInst* fInst, Map* typeMap);        // TODO:

// Statement
int statement(FileInst* fInst, Map* typeMap);             // TODO:
int labeled_statement(FileInst* fInst, Map* typeMap);     // TODO:
int compound_statement(FileInst* fInst, Map* typeMap);    // TODO:
int block_item_list(FileInst* fInst, Map* typeMap);       // TODO:
int block_item(FileInst* fInst, Map* typeMap);            // TODO:
int expression_statement(FileInst* fInst, Map* typeMap);  // TODO:
int selection_statement(FileInst* fInst, Map* typeMap);   // TODO:
int iteration_statement(FileInst* fInst, Map* typeMap);   // TODO:
int jump_statement(FileInst* fInst, Map* typeMap);        // TODO:
#endif