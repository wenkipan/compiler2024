#pragma once

#include <util/util.hpp>
#include <symbol/symbol.hpp>
#include <symbol/var.hpp>
typedef class symbol_func *p_symbol_func;
class symbol_func
{
public:
    // type info
    bool is_va;
    basic_type ret_type;

    char *name;
    uint64_t id;

    size_t var_cnt;
    list_head param;
    list_head variable;

    size_t param_reg_cnt;
    list_head param_reg_list;

    size_t vreg_cnt;
    list_head vreg_list;

    size_t block_cnt;
    list_head block;
    // p_ir_basic_block p_ret_block;
    // p_ir_basic_block p_entry_block;

    list_head call_param_vmem_list;
    size_t stack_size;
    size_t inner_stack_size;

    // p_nestedtree_node p_nestedtree_root;

    size_t instr_num;

    // size_t use_reg_num_r;
    // size_t use_reg_num_s;
    // size_t save_reg_r_num;
    // size_t save_reg_s_num;

    list_head node;

    bool if_updated_graph;
    // gen

    // p_symbol_func symbol_func_gen(const char *name, basic_type b_type, bool is_va);
    symbol_func(const char *name, basic_type b_type, bool is_va);
    // void symbol_func_drop(p_symbol_func p_func);
    ~symbol_func();

    void symbol_func_add_variable(p_symbol_var p_var);
    void symbol_func_add_call_param_vmem(p_symbol_var p_vmem);
    void symbol_func_add_param(p_symbol_var p_var);

    // void symbol_func_bb_add_head(p_ir_basic_block p_basic_block);
    // void symbol_func_bb_add_tail(p_ir_basic_block p_basic_block);
    // void symbol_func_bb_del(p_ir_basic_block p_basic_block);

    // void symbol_func_param_reg_add(p_ir_vreg p_vreg);
    // void symbol_func_param_reg_del(p_ir_vreg p_vreg);

    // p_symbol_var symbol_func_param_reg_mem(p_ir_vreg p_vreg);

    // void symbol_func_vreg_add(p_ir_vreg p_vreg);
    // void symbol_func_vreg_del(p_ir_vreg p_vreg);
    // void symbol_func_basic_block_init_visited();
    // void symbol_func_set_block_id();

    // void symbol_func_clear_varible();
    // void symbol_func_delete_varible(p_symbol_var p_var);
    // void symbol_func_set_varible_id();
    // // print
    void symbol_func_name_print();
    void symbol_func_init_print();
};