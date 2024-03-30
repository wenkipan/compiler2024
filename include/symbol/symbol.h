#pragma once

#include <util.h>
#include <ir.h>
typedef class symbol_init_val *p_symbol_init_val;
typedef class symbol_init *p_symbol_init;
typedef class symbol_var *p_symbol_var;
typedef class symbol_func *p_symbol_func;
typedef class symbol_str *p_symbol_str;

typedef enum
{
    type_void,
    type_str,
    type_i32,
    type_f32,
} basic_type;
typedef class symbol_type *p_symbol_type;
typedef class symbol_type_array *p_symbol_type_array;

class symbol_init_val
{
    union
    {
        I32CONST_t i;
        F32CONST_t f;
    };
};

class symbol_init
{
    basic_type basic;
    size_t size;
    p_symbol_init_val memory;
};

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
    p_ir_basic_block p_ret_block;
    p_ir_basic_block p_entry_block;

    list_head call_param_vmem_list;
    size_t stack_size;
    size_t inner_stack_size;

    p_nestedtree_node p_nestedtree_root;

    size_t instr_num;

    size_t use_reg_num_r;
    size_t use_reg_num_s;
    size_t save_reg_r_num;
    size_t save_reg_s_num;
    list_head node;

    bool if_updated_graph;
    // gen
    p_symbol_func symbol_func_gen(const char *name, basic_type b_type, bool is_va);

    void symbol_func_add_variable(p_symbol_func p_func, p_symbol_var p_var);
    void symbol_func_add_call_param_vmem(p_symbol_func p_func, p_symbol_var p_vmem);
    void symbol_func_add_param(p_symbol_func p_func, p_symbol_var p_var);

    void symbol_func_drop(p_symbol_func p_func);

    void symbol_func_bb_add_head(p_symbol_func p_func, p_ir_basic_block p_basic_block);
    void symbol_func_bb_add_tail(p_symbol_func p_func, p_ir_basic_block p_basic_block);
    void symbol_func_bb_del(p_symbol_func p_func, p_ir_basic_block p_basic_block);
    void symbol_func_param_reg_add(p_symbol_func p_func, p_ir_vreg p_vreg);
    void symbol_func_param_reg_del(p_symbol_func p_func, p_ir_vreg p_vreg);
    p_symbol_var symbol_func_param_reg_mem(p_symbol_func p_func, p_ir_vreg p_vreg);
    void symbol_func_vreg_add(p_symbol_func p_func, p_ir_vreg p_vreg);
    void symbol_func_vreg_del(p_symbol_func p_func, p_ir_vreg p_vreg);
    void symbol_func_basic_block_init_visited(p_symbol_func p_func);
    void symbol_func_set_block_id(p_symbol_func p_func);

    void symbol_func_clear_varible(p_symbol_func p_func);
    void symbol_func_delete_varible(p_symbol_func p_func, p_symbol_var p_var);
    void symbol_func_set_varible_id(p_symbol_func p_func);
    // print
    void symbol_func_name_print(p_symbol_func p_func);
    void symbol_func_init_print(p_symbol_func p_func);
};

class symbol_str
{
    char *string;
    size_t length;

    hlist_node h_node;
    list_head node;
    // gen
    p_symbol_str symbol_str_gen(const char *string);
    void symbol_str_drop(p_symbol_str p_str);
    // print
    void symbol_str_print(p_symbol_str p_str);
};

class symbol_init_val
{
    union
    {
        I32CONST_t i;
        F32CONST_t f;
    };
};

class symbol_init
{
    basic_type basic;
    size_t size;
    p_symbol_init_val memory;
    p_symbol_init symbol_init_gen(size_t size, basic_type basic);
    p_symbol_init symbol_init_add(p_symbol_init p_init, size_t offset, symbol_init_val val);
    void symbol_init_drop(p_symbol_init p_init);
};

class symbol_var
{
public:
    // type info
    p_symbol_type p_type;

    char *name;
    uint64_t id;

    p_symbol_init p_init;
    bool is_global;
    bool is_const;

    size_t stack_offset;
    list_head node;
    // gen
    p_symbol_var symbol_var_gen(const char *name, p_symbol_type p_type, bool is_const, bool is_global, p_symbol_init p_data);
    p_symbol_var symbol_temp_var_gen(p_symbol_type p_type);

    void symbol_var_drop(p_symbol_var p_var);

    // print
    void symbol_name_print(p_symbol_var p_var);
    void symbol_init_print(p_symbol_var p_var);
};

// maybe problem!!
uint64_t basic_type_get_size(basic_type b_type);
uint64_t symbol_type_get_size(p_symbol_type p_type);
uint64_t symbol_type_array_get_size(p_symbol_type_array p_array);
class symbol_type_array
{
public:
    uint64_t size;
    list_head node;

    void symbol_type_push_array(p_symbol_type p_type, p_symbol_type_array p_array);
    p_symbol_type_array symbol_type_top_array(p_symbol_type p_type);
    p_symbol_type_array symbol_type_pop_array(p_symbol_type p_type);

    p_symbol_type_array symbol_type_array_copy(p_symbol_type_array p_array);

    p_symbol_type_array symbol_type_array_gen(size_t size);

    void symbol_type_array_drop(p_symbol_type_array p_array);
};

class symbol_type
{
public:
    list_head array;
    uint64_t ref_level;
    basic_type basic;
    uint64_t size;

    void symbol_type_push_ptr(p_symbol_type p_type);
    void symbol_type_pop_ptr(p_symbol_type p_type);

    p_symbol_type symbol_type_copy(p_symbol_type p_type);

    p_symbol_type symbol_type_var_gen(basic_type basic);

    void symbol_type_drop(p_symbol_type p_type);
};
// dont sure where to belong
void symbol_basic_type_print(basic_type b_type);
void symbol_type_print(p_symbol_type p_type);