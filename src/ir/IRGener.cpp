#include <ir/IRGener.hpp>

GenFunction::GenFunction(Function *p_func, BasicBlock *p_block)
    : func(p_func), curBB(p_block)
{
}

void GenFunction::Func_add_BB(BasicBlock *p_BB)
{
    func->BB_addTail(p_BB);
    curBB = p_BB;
}

void GenFunction::BB_add_Ins(Instrution *p_instr)
{
    curBB->Ins_pushBack(p_instr);
    func->Val_pushBack(p_instr);
}

void GenFunction::stmt2ir(p_ast_block p_block)
{
    p_list_head p_node;
    list_for_each(p_node, &p_block->stmt)
    {
        p_ast_stmt p_stmt = list_entry(p_node, ast_stmt, node);
        ir_stmt_gen(nullptr, nullptr, p_stmt);
    }
}

void GenFunction::ir_stmt_gen(BasicBlock *p_start, BasicBlock *p_next, p_ast_stmt p_stmt)
{
    if (!p_stmt)
        return;
    switch (p_stmt->type)
    {
    case ast_stmt::ast_stmt_assign:
        ast2ir_stmt_assign_gen(p_stmt->array.p_lval, p_stmt->array.p_rval, p_stmt->array.is_stack);
        break;
    case ast_stmt::ast_stmt_return:
        break;
    case ast_stmt::ast_stmt_exp:
        break;
    case ast_stmt::ast_stmt_block:
        break;
    case ast_stmt::ast_stmt_if_else:
        break;
    case ast_stmt::ast_stmt_if:
        break;
    case ast_stmt::ast_stmt_while:
        break;
    case ast_stmt::ast_stmt_break:
        break;
    case ast_stmt::ast_stmt_continue:
        break;
    default:
        assert(0);
    }
}

void GenFunction::ast2ir_stmt_assign_gen(p_ast_exp p_lval, p_ast_exp p_rval, bool is_stack)
{
}