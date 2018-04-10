#ifndef WVMCC_PARSE_RULES_DEF
#define WVMCC_PARSE_RULES_DEF

#include "../fileInst.h"
#include "structs.h"
#include "token.h"

// Return -1 if error, 0 if success
int startParse(FileInst* fInst);  

// Return 1 if match, 0 if not match, but not indicate whether error
// happened or not

// External definitions
int translation_unit(FileInst* fInst, Map* typeMap, List *declList, List *codeList);      
int external_declaration(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int function_definition(FileInst* fInst, Map* typeMap, List *declList);   
int declaration_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);      
int preprocessor_hint(FileInst* fInst);

// Declarations
int declaration(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int declaration_specifiers(FileInst* fInst,
                           Map* typeMap,
                           Declaration** declPtr, List *declList, List *codeList);           
int init_declarator_list(FileInst* fInst, Map* typeMap, Declaration *decl, List *declList, List *codeList);  
int init_declarator(FileInst* fInst, Map* typeMap, Declaration *decl, List *declList, List *codeList);       
int storage_class_specifier(FileInst* fInst, Map* typeMap, Declaration* decl);
int type_specifier(FileInst* fInst, Map* typeMap, Declaration** declPtr, List *declList, List *codeList);  
int struct_or_union_specifier(FileInst* fInst,
                              Map* typeMap,
                              Declaration** declPtr, List *declList, List *codeList);  
int struct_or_union(FileInst* fInst, Map* typeMap, Declaration* decl);
int struct_declaration_list(FileInst* fInst,
                            Map* typeMap,
                            Declaration** declPtr, List *declList, List *codeList);  
int struct_declaration(FileInst* fInst,
                       Map* typeMap,
                       Declaration** declPtr, List *declList, List *codeList);  
int specifier_qualifier_list(FileInst* fInst,
                             Map* typeMap,
                             Declaration** declPtr, List *declList, List *codeList);  
int struct_declarator_list(FileInst* fInst,
                           Map* typeMap,
                           Declaration** declPtr, List *declList, List *codeList);  
int struct_declarator(FileInst* fInst,
                      Map* typeMap,
                      Declaration** declPtr, List *declList, List *codeList);                 
int enum_specifier(FileInst* fInst, Map* typeMap, List *declList, List *codeList);         
int enumerator_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
int enumerator(FileInst* fInst, Map* typeMap, List *declList, List *codeList);             
int atomic_type_specifier(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int type_qualifier(FileInst* fInst, Map* typeMap, char *qualifier);
int function_specifier(FileInst* fInst, Map* typeMap);   
int alignment_specifier(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int declarator(FileInst* fInst, Map* typeMap, Declaration *decl, List *declList, List *codeList);           
int direct_declarator(FileInst* fInst, Map* typeMap, Declaration *decl, List *declList, List *codeList);    
int pointer(FileInst* fInst, Map* typeMap, List *ptrList);
int type_qualifier_list(FileInst* fInst, Map* typeMap, char *qualifier);  
int parameter_type_list(FileInst* fInst,
                        Map* typeMap,
                        Declaration** declPtr, List *declList, List *codeList);                            
int parameter_list(FileInst* fInst, Map* typeMap, ParamDeclarator* paramDecl, List *declList, List *codeList);  
int parameter_declaration(FileInst* fInst,
                          Map* typeMap,
                          Declaration** declPtr, List *declList, List *codeList);       
int identifier_list(FileInst* fInst, Map* typeMap);  
int type_name(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
int abstract_declarator(FileInst* fInst,
                        Map* typeMap,
                        Declaration** declPtr, List *declList, List *codeList);  
int direct_abstract_declarator(FileInst* fInst,
                               Map* typeMap,
                               Declaration** declPtr, List *declList, List *codeList);            
int typedef_name(FileInst* fInst, Map* typeMap);               
int initializer(FileInst* fInst, Map* typeMap, List *declList, List *codeList);                
int initializer_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);           
int designation(FileInst* fInst, Map* typeMap, List *declList, List *codeList);                
int designator_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);            
int designtor(FileInst* fInst, Map* typeMap, List *declList, List *codeList);                  
int static_assert_declaration(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  

// Expressions
int primary_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);         
int generic_selection(FileInst* fInst, Map* typeMap, List *declList, List *codeList);          
int generic_assoc_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);         
int generic_association(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
int postfix_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);         
int argument_expression_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);   
int unary_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);           
int unary_operator(FileInst* fInst, Map* typeMap);             
int cast_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);            
int multiplicative_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int additive_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
int shift_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);           
int relational_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);      
int equality_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
int and_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);             
int exclusive_or_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);    
int inclusive_or_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);    
int logical_and_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);     
int logical_or_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);      
int conditional_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);     
int assignment_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);      
int assignment_operator(FileInst* fInst, Map* typeMap);        
int expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);                 
int constant_expression(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        

// Statement
int statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);             
int labeled_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);     
int compound_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);    
int block_item_list(FileInst* fInst, Map* typeMap, List *declList, List *codeList);       
int block_item(FileInst* fInst, Map* typeMap, List *declList, List *codeList);            
int expression_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);  
int selection_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);   
int iteration_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);   
int jump_statement(FileInst* fInst, Map* typeMap, List *declList, List *codeList);        
#endif