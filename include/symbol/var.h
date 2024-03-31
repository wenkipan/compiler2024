#pragma once

#include <util.h>
#include <ir.h>

class symbol_init_val
{
public:
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