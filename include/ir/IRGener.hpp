#pragma once

#include <ir/ir.hpp>

class GenFunction
{
public:
    Function *func;
    BasicBlock *curBB;

public:
    GenFunction(Function *p_func, BasicBlock *p_block);

    void stmt2ir(p_ast_block p_block);
    void ir_stmt_gen(BasicBlock *p_start, BasicBlock *p_next, p_ast_stmt p_stmt);
    void ast2ir_stmt_assign_gen(p_ast_exp p_lval, p_ast_exp p_rval, bool is_stack);

    void Func_add_BB(BasicBlock *p_BB);
    void BB_add_Ins(Instrution *p_instr);
};