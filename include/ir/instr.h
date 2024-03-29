#pragma once
#include <ir.h>
enum ir_binary_op
{
    // binary instr
    ir_add_op,
    ir_sub_op,
    ir_mul_op,
    ir_div_op,
    ir_mod_op,
    ir_eq_op,
    ir_neq_op,
    ir_l_op,
    ir_leq_op,
    ir_g_op,
    ir_geq_op,
};

enum ir_unary_op
{
    // unary instr
    ir_minus_op,
    ir_val_assign,
    ir_i2f_op,
    ir_f2i_op,
    ir_ptr_add_sp,
};

enum ir_instr_type
{
    // binary instr
    ir_binary,

    // unary instr
    ir_unary,

    // memory
    ir_gep,
    ir_store,
    ir_load,
    // func call
    ir_call,
};

class ir_binary_instr
{
    ir_binary_op op;
    p_ir_operand p_src1, p_src2;
    p_ir_vreg p_des;
};

class ir_unary_instr
{
    ir_unary_op op;
    p_ir_operand p_src;
    p_ir_vreg p_des;
};

class ir_call_instr
{
    p_symbol_func p_func;
    list_head param_list;
    p_ir_instr p_first_store;
    p_ir_vreg p_des;
};

class ir_gep_instr
{
    p_ir_operand p_addr;
    p_ir_operand p_offset;
    p_ir_vreg p_des;
    bool is_element;
};
class ir_load_instr
{
    bool is_stack_ptr;
    p_ir_operand p_addr;
    p_ir_vreg p_des;
};
class ir_store_instr
{
    bool is_stack_ptr;
    p_ir_operand p_addr;
    p_ir_operand p_src;
};

class ir_instr
{
    ir_instr_type irkind;
    union
    {
        ir_binary_instr ir_binary;
        ir_unary_instr ir_unary;
        ir_call_instr ir_call;
        ir_load_instr ir_load;
        ir_gep_instr ir_gep;
        ir_store_instr ir_store;
    };
    size_t instr_id;
    p_ir_vreg_list p_live_in;
    p_ir_vreg_list p_live_out;
    p_ir_basic_block p_basic_block;
    list_head node; // 下一条指令

    p_ir_instr ir_binary_instr_gen(ir_binary_op op, p_ir_operand p_src1, p_ir_operand p_src2, p_ir_vreg des);
    p_ir_instr ir_unary_instr_gen(ir_unary_op op, p_ir_operand p_src, p_ir_vreg p_des);

    p_ir_instr ir_call_instr_gen(p_symbol_func p_func, p_ir_vreg p_des);
    void ir_call_param_list_add(p_ir_instr p_instr, p_ir_operand p_param);

    p_ir_instr ir_gep_instr_gen(p_ir_operand p_addr, p_ir_operand p_offset, p_ir_vreg p_des, bool is_element);
    p_ir_instr ir_load_instr_gen(p_ir_operand p_addr, p_ir_vreg p_des, bool is_stack_ptr);
    p_ir_instr ir_store_instr_gen(p_ir_operand p_addr, p_ir_operand p_src, bool is_stack_ptr);

    void ir_instr_reset_binary(p_ir_instr p_instr, ir_binary_op op, p_ir_operand p_src1, p_ir_operand p_src2, p_ir_vreg p_des);
    void ir_instr_reset_unary(p_ir_instr p_instr, ir_unary_op op, p_ir_operand p_src, p_ir_vreg p_des);
    void ir_instr_reset_call(p_ir_instr p_instr, p_symbol_func p_func, p_ir_vreg p_des);
    void ir_instr_reset_gep(p_ir_instr p_instr, p_ir_operand p_addr, p_ir_operand p_offset, p_ir_vreg p_des, bool is_element);
    void ir_instr_reset_load(p_ir_instr p_instr, p_ir_operand p_addr, p_ir_vreg p_des, bool is_stack_ptr);
    void ir_instr_reset_store(p_ir_instr p_instr, p_ir_operand p_addr, p_ir_operand p_src, bool is_stack_ptr);

    void ir_instr_add_next(p_ir_instr p_next, p_ir_instr p_prev);
    void ir_instr_add_prev(p_ir_instr p_prev, p_ir_instr p_next);

    p_ir_operand ir_instr_get_src1(p_ir_instr p_instr);
    p_ir_operand ir_instr_get_src2(p_ir_instr p_instr);
    p_ir_vreg ir_instr_get_des(p_ir_instr p_instr);
    p_ir_operand ir_instr_get_load_addr(p_ir_instr p_instr);
    p_ir_operand ir_instr_get_store_addr(p_ir_instr p_instr);

    void ir_set_load_instr_des(p_ir_instr p_load, p_ir_vreg p_des);
    void ir_set_load_instr_addr(p_ir_instr p_load, p_ir_operand p_addr);
    void ir_set_binary_instr_des(p_ir_instr p_binary, p_ir_vreg p_des);
    void ir_set_binary_instr_src1(p_ir_instr p_load, p_ir_operand p_src1);
    void ir_set_binary_instr_src2(p_ir_instr p_load, p_ir_operand p_src2);
    void ir_set_unary_instr_des(p_ir_instr p_unary, p_ir_vreg p_des);
    void ir_set_unary_instr_src(p_ir_instr p_unary, p_ir_operand p_src);
    void ir_set_gep_instr_des(p_ir_instr p_gep, p_ir_vreg p_des);
    void ir_set_gep_instr_addr(p_ir_instr p_gep, p_ir_operand p_addr);
    void ir_set_gep_instr_offset(p_ir_instr p_gep, p_ir_operand p_offset);
    void ir_set_call_instr_des(p_ir_instr p_call, p_ir_vreg p_des);
    void ir_set_store_instr_src(p_ir_instr p_store, p_ir_operand p_src);
    void ir_set_store_instr_offset(p_ir_instr p_store, p_ir_operand p_offset);

    void ir_instr_del(p_ir_instr p_instr);
    void ir_instr_drop(p_ir_instr p_instr);
};