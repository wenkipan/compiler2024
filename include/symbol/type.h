#pragma once

#include <util.h>
#include <ir.h>

uint64_t basic_type_get_size(basic_type b_type);
uint64_t symbol_type_get_size(p_symbol_type p_type);
uint64_t symbol_type_array_get_size(p_symbol_type_array p_array);
class symbol_type_array
{
public:
    uint64_t size;
    list_head node;

    void symbol_type_push_array(p_symbol_type p_type, p_symbol_type_array p_array);

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

    // p_symbol_type symbol_type_var_gen(basic_type basic);
    symbol_type(basic_type basic);

    void symbol_type_push_ptr();
    void symbol_type_pop_ptr();

    p_symbol_type_array symbol_type_top_array(p_symbol_type p_type);
    p_symbol_type_array symbol_type_pop_array(p_symbol_type p_type);

    p_symbol_type symbol_type_copy();

    // void symbol_type_drop(p_symbol_type p_type);
    ~symbol_type();
};