#include "rules.h"

int primary_expression(FileInst* fInst){
    
}

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