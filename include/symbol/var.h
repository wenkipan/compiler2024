#pragma once

#include <util.hpp>
#include <symbol.h>

typedef class symbol_init_val *p_symbol_init_val;
typedef class symbol_init *p_symbol_init;
typedef class symbol_var *p_symbol_var;
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
public:
    basic_type basic;
    size_t size;
    p_symbol_init_val memory;
    // p_symbol_init symbol_init_gen(size_t size, basic_type basic);
    symbol_init(size_t size, basic_type basic);
    // void symbol_init_drop(p_symbol_init p_init);
    ~symbol_init();

    p_symbol_init symbol_init_add(size_t offset, symbol_init_val val);
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
    // p_symbol_var symbol_var_gen(const char *name, p_symbol_type p_type, bool is_const, bool is_global, p_symbol_init p_data);
    symbol_var(const char *name, p_symbol_type p_type, bool is_const, bool is_global, p_symbol_init p_data);
    // p_symbol_var symbol_temp_var_gen(p_symbol_type p_type);
    symbol_var(p_symbol_type p_type);

    // void symbol_var_drop(p_symbol_var p_var);
    ~symbol_var();
    // print
    void symbol_name_print();
    void symbol_init_print();
};