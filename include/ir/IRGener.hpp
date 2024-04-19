#pragma once

#include <ir/ir.hpp>
#include <unordered_map>

class Module;

class GenFunction
{

public:
    static Module *module;
    static std::unordered_map<p_symbol_var, Value *> *_global_map;
    Function *func;
    BasicBlock *curBB;
    BasicBlock *retBB;

    Value *p_ret;

    std::unordered_map<p_symbol_var, Value *> _map;

public:
    GenFunction(Function *p_func, BasicBlock *p_block, p_symbol_func _p_func);

    void new_curBB();
    void set_curBB(BasicBlock *_BB) { curBB = _BB; };

    void stmt2ir(p_ast_block p_block);
    void ast2ir_stmt_gen(BasicBlock *p_start, BasicBlock *p_next, p_ast_stmt p_stmt);

    void ast2ir_stmt_assign_gen(p_ast_exp p_lval, p_ast_exp p_rval, bool is_stack);
    void ast2ir_stmt_return_gen(p_ast_exp p_exp);
    void ast2ir_stmt_exp_gen(p_ast_exp p_exp);
    void ast2ir_stmt_block_gen(BasicBlock *while_start, BasicBlock *while_end, p_ast_block p_block);
    void ast2ir_stmt_if_gen(BasicBlock *p_cond, BasicBlock *p_next, p_ast_exp p_exp, p_ast_stmt p_stmt);
    void ast2ir_stmt_if_else_gen(BasicBlock *p_while_cond, BasicBlock *p_while_end_next, p_ast_exp p_exp, p_ast_stmt p_stmt1, p_ast_stmt p_stmt2);
    void ast2ir_stmt_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt1);
    void ast2ir_stmt_break_gen(BasicBlock *p_while_end_next);
    void ast2ir_stmt_continue_gen(BasicBlock *p_while_cond);

    Value *ast2ir_exp_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_num_gen(p_ast_exp p_exp);
    Value *get_addr(p_symbol_var p_var, p_symbol_type p_type, I32CONST_t offset);
    Value *ast2ir_exp_gep_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_load_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_relational_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_binary_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_unary_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_logic_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_ulogic_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_call_gen(p_ast_exp p_exp);
    Value *ast2ir_exp_use_gen(p_ast_exp p_exp);
    void ast2ir_exp_cond_gen(BasicBlock *p_trueblock, BasicBlock *p_nextblock, p_ast_exp p_exp);

    void Func_add_BB(BasicBlock *p_BB);
};