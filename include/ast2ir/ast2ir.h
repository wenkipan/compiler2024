#pragma once
#include <ir.h>
#include <program.h>
#include <ast.h>
typedef struct ast2ir_info *p_ast2ir_info;
class ast2ir_info
{
    p_ir_basic_block p_current_basic_block;
    p_symbol_func p_func;
    p_program p_program;

    p_ir_basic_block p_ret_block; // 返回的块
    p_symbol_var p_ret_vmem;      // 返回值所存储的位置

    p_ast2ir_info ast2ir_info_gen(p_symbol_func p_m_func, p_program p_program);

    void ast2ir_info_add_instr(p_ast2ir_info p_info, p_ir_instr p_instr);
    void ast2ir_info_add_basic_block(p_ast2ir_info p_info, p_ir_basic_block p_basic_block);

    void ast2ir_info_drop(p_ast2ir_info p_info);
};

// block
void ast2ir_block_gen(p_ast2ir_info p_info, p_ast_block p_block);
// exp
p_ir_operand ast2ir_exp_gen(p_ast2ir_info p_info, p_ast_exp p_exp);
p_ir_operand ast2ir_exp_cond_gen(p_ast2ir_info p_info, p_ir_basic_block p_true_block, p_ir_basic_block p_false_block, p_ast_exp p_exp);

// func
void ast2ir_symbol_func_gen(p_ast_block p_block, p_symbol_func p_m_func, p_program p_program);

// param
p_ir_param_list ast2ir_param_list_gen(p_ast2ir_info p_info, p_ast_param_list p_param_list);

// stmt

void ast2ir_stmt_gen(p_ast2ir_info p_info, p_ir_basic_block while_start, p_ir_basic_block while_end_next, p_ast_stmt p_stmt);

void ast2ir_stmt_return_gen(p_ast2ir_info p_info, p_ast_exp p_exp);
void ast2ir_stmt_exp_gen(p_ast2ir_info p_info, p_ast_exp p_exp);
void ast2ir_stmt_break_gen(p_ast2ir_info p_info, p_ir_basic_block p_while_end_next);
void ast2ir_stmt_continue_gen(p_ast2ir_info p_info, p_ir_basic_block p_while_start);
void ast2ir_stmt_if_gen(p_ast2ir_info p_info, p_ir_basic_block p_while_start, p_ir_basic_block p_while_end_next, p_ast_exp p_exp, p_ast_stmt p_stmt_1);
void ast2ir_stmt_if_else_gen(p_ast2ir_info p_info, p_ir_basic_block p_while_start, p_ir_basic_block p_while_end_next, p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2);
void ast2ir_stmt_while_gen(p_ast2ir_info p_info, p_ast_exp p_exp, p_ast_stmt p_stmt_1);
void ast2ir_stmt_assign_gen(p_ast2ir_info p_info, p_ast_exp p_lval, p_ast_exp p_rval, bool is_stack);
void ast2ir_stmt_block_gen(p_ast2ir_info p_info, p_ir_basic_block while_start, p_ir_basic_block while_end_next, p_ast_block p_block);