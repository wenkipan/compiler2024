#pragma once
#ifndef __AST_BLOCK__
#define __AST_BLOCK__

#include <util/util.hpp>
#include <symbol/symbol.hpp>
#include <symbol/var.hpp>
#include <symbol/func.hpp>
#include <symbol/str.hpp>
// #include <ir.hpp>
typedef class ast_param *p_ast_param;
typedef class ast_param_list *p_ast_param_list;
typedef class ast_func *p_ast_func;
typedef class ast_block *p_ast_block;
typedef class ast_stmt *p_ast_stmt;
typedef class ast_exp *p_ast_exp;

enum ast_exp_logic_op
{
    ast_exp_op_bool_or,
    ast_exp_op_bool_and,
};
enum ast_exp_ulogic_op
{
    ast_exp_op_bool_not,
};

enum ast_exp_relational_op
{
    ast_exp_op_eq,
    ast_exp_op_neq,
    ast_exp_op_l,
    ast_exp_op_leq,
    ast_exp_op_g,
    ast_exp_op_geq,
};
enum ast_exp_binary_op
{
    ast_exp_op_add,
    ast_exp_op_sub,
    ast_exp_op_mul,
    ast_exp_op_div,
    ast_exp_op_mod,
};
enum ast_exp_unary_op
{
    ast_exp_op_minus,
    ast_exp_op_i2f,
    ast_exp_op_f2i,
};
class ast_block
{
public:
    uint64_t length;
    list_head stmt;

    // initialize
    // p_ast_block ast_block_gen(void);
    ast_block();
    p_ast_block ast_block_add(p_ast_stmt p_stmt);
    // void ast_block_drop(p_ast_block p_block);
    ~ast_block();
};

class ast_param_list
{
public:
    list_head param;
    ast_param_list();
    p_ast_param_list ast_param_list_add(p_ast_exp p_exp);
    ~ast_param_list();
    // p_ast_param_list ast_param_list_init(void);
    // p_ast_param_list ast_param_list_add(p_ast_param_list p_head, p_ast_exp p_exp);
    // void ast_param_list_drop(p_ast_param_list p_param_list);
};

class ast_param
{
public:
    bool is_stck_ptr;
    p_ast_exp p_exp;
    list_head node;
    ast_param(p_ast_exp p_exp);
    ~ast_param();
};

class ast_stmt
{
public:
    enum stmt_type
    {
        ast_stmt_assign,
        ast_stmt_return,
        ast_stmt_exp,     // p_exp (can null)
        ast_stmt_block,   // p_block
        ast_stmt_if_else, // p_exp p_stmt_1 p_stmt_2
        ast_stmt_if,
        ast_stmt_while, // p_exp p_stmt_1
        ast_stmt_break,
        ast_stmt_continue, // null
    } type;

    union
    {
        struct
        {
            p_ast_exp p_exp;
            p_ast_stmt p_stmt_1, p_stmt_2;
        } branch;
        struct
        {
            bool is_stack;
            p_ast_exp p_lval, p_rval;
        } array;
        p_ast_block p_block;
        void *null;
    };

    list_head node;
    ast_stmt(basic_type ret_type, p_ast_exp p_exp);
    ast_stmt(p_ast_exp p_exp);
    ast_stmt();
    ast_stmt(p_ast_exp p_exp, p_ast_stmt p_stmt_1);
    ast_stmt(p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2);
    ast_stmt(p_ast_block p_block);
    ast_stmt(p_ast_exp lval, p_ast_exp rval);
    // void ast_stmt_drop(p_ast_stmt p_stmt);
    ~ast_stmt();
};
p_ast_stmt ast_stmt_return_gen(basic_type ret_type, p_ast_exp p_exp);
p_ast_stmt ast_stmt_exp_gen(p_ast_exp p_exp);
p_ast_stmt ast_stmt_break_gen(void);    // cant  ignore
p_ast_stmt ast_stmt_continue_gen(void); // cant  ignore
p_ast_stmt ast_stmt_if_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1);
p_ast_stmt ast_stmt_if_else_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1, p_ast_stmt p_stmt_2);
p_ast_stmt ast_stmt_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1);
p_ast_stmt ast_stmt_do_while_gen(p_ast_exp p_exp, p_ast_stmt p_stmt_1);
p_ast_stmt ast_stmt_block_gen(p_ast_block p_block); // special
p_ast_stmt ast_stmt_assign_gen(p_ast_exp lval, p_ast_exp rval);

class ast_exp
{
public:
    union
    {
        struct
        {
            p_ast_exp p_rsrc_1, p_rsrc_2;
            ast_exp_relational_op r_op;
        } r; // relational
        struct
        {
            p_ast_exp p_src_1, p_src_2;
            ast_exp_binary_op b_op;
        } b; // binary
        struct
        {
            p_ast_exp p_src;
            ast_exp_unary_op u_op;
        } u; // unary
        struct
        {
            p_ast_exp p_bool_1, p_bool_2;
            ast_exp_logic_op l_op;
        } l; // logic
        struct
        {
            p_ast_exp p_bool;
            ast_exp_ulogic_op ul_op;
        } ul; // ulogic
        struct
        {
            p_symbol_func p_func;
            p_ast_param_list p_param_list;
        } call; // call
        struct
        {
            bool is_element;
            bool is_stack_for_gep;
            p_ast_exp p_addr;
            p_ast_exp p_offset;
        } gep; // gep
        struct
        {
            bool is_stack;
            p_ast_exp p_ptr;
        } load; // load
        union
        {
            I32CONST_t i32const; // int
            F32CONST_t f32const; // float
            p_symbol_str p_str;  // str
        };                       // null
        p_ast_exp p_exp;         // use
        p_symbol_var p_var;      // ptr
    };

    enum
    {
        ast_exp_binary,
        ast_exp_relational,
        ast_exp_unary,
        ast_exp_logic,
        ast_exp_ulogic,
        ast_exp_use,
        ast_exp_call,
        ast_exp_ptr,
        ast_exp_gep,
        ast_exp_load,
        ast_exp_num,
    } kind;
    p_symbol_type p_type;

    // p_ir_vreg p_des;

    ast_exp() {}
    // p_ast_exp ast_exp_relational_gen(ast_exp_relational_op op, p_ast_exp p_rsrc_1, p_ast_exp p_rsrc_2);
    ast_exp(ast_exp_relational_op op, p_ast_exp p_rsrc_1, p_ast_exp p_rsrc_2);
    // p_ast_exp ast_exp_logic_gen(ast_exp_logic_op l_op, p_ast_exp p_bool_1, p_ast_exp p_bool_2);
    ast_exp(ast_exp_logic_op l_op, p_ast_exp p_bool_1, p_ast_exp p_bool_2);
    // p_ast_exp ast_exp_ulogic_gen(ast_exp_ulogic_op ul_op, p_ast_exp p_bool);
    ast_exp(ast_exp_ulogic_op ul_op, p_ast_exp p_bool);
    // p_ast_exp ast_exp_call_gen(p_symbol_func p_func, p_ast_param_list p_param_list);
    ast_exp(p_symbol_func p_func, p_ast_param_list p_param_list);
    // p_ast_exp ast_exp_ptr_gen(p_symbol_var p_var);
    ast_exp(p_symbol_var p_var);
    // p_ast_exp ast_exp_gep_gen(p_ast_exp p_val, p_ast_exp p_offset, bool is_element);
    ast_exp(p_ast_exp p_val, p_ast_exp p_offset, bool is_element);
    // p_ast_exp ast_exp_load_gen(p_ast_exp p_ptr);
    ast_exp(p_ast_exp p_ptr);
    p_ast_exp ast_exp_load_gen();
    // p_ast_exp ast_exp_int_gen(I32CONST_t num);
    ast_exp(I32CONST_t num);
    // p_ast_exp ast_exp_float_gen(F32CONST_t num);
    ast_exp(F32CONST_t num);
    // p_ast_exp ast_exp_str_gen(p_symbol_str p_str);
    ast_exp(p_symbol_str p_str);
    // support ast_exp_binary_gen
    ast_exp(ast_exp_binary_op b_op, p_ast_exp p_src_1, p_ast_exp p_src_2);
    // support ast_exp_unary_gen
    ast_exp(ast_exp_unary_op u_op, p_ast_exp p_src);

    p_ast_exp ast_exp_use_gen();
    // delete void ast_exp_drop(p_ast_exp p_exp);
    ~ast_exp();

    void ast_exp_ptr_check_lval();
    // p_ast_exp ast_exp_ptr_check_const();
    p_ast_exp ast_exp_ptr_to_val_check_basic();
    p_ast_exp ast_exp_ptr_to_val();
    bool ast_exp_ptr_is_stack();
    p_ast_exp ast_exp_cov_gen(basic_type b_type);
    p_ast_exp ast_exp_to_cond();
};
// hard to change
p_ast_exp ast_exp_ptr_check_const(p_ast_exp p_exp);
p_ast_exp syntax_val_offset(p_ast_exp p_val, p_ast_exp p_offset);

p_ast_exp ast_exp_binary_gen(ast_exp_binary_op op, p_ast_exp p_src_1, p_ast_exp p_src_2);
p_ast_exp ast_exp_unary_gen(ast_exp_unary_op op, p_ast_exp p_src);

#endif