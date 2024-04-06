#pragma once

#include <util/util.hpp>
#include <symbol/symbol.hpp>
typedef class symbol_type_array *p_symbol_type_array;
typedef class symbol_type *p_symbol_type;
class symbol_type_array
{
public:
    uint64_t size;
    list_head node;

    // p_symbol_type_array symbol_type_array_gen(size_t size);
    symbol_type_array(size_t size);
    // p_symbol_type_array symbol_type_array_copy(p_symbol_type_array p_array);
    symbol_type_array(p_symbol_type_array p_array);
    void symbol_type_push_array(p_symbol_type p_type);
    // useless
    //  void symbol_type_array_drop(p_symbol_type_array p_array);
    ~symbol_type_array() {}
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

    p_symbol_type_array symbol_type_top_array();
    p_symbol_type_array symbol_type_pop_array();

    // another new object
    p_symbol_type symbol_type_copy();

    // void symbol_type_drop(p_symbol_type p_type);
    ~symbol_type();

    // uint64_t symbol_type_get_size(p_symbol_type p_type);
    void symbol_type_print();
};
void symbol_basic_type_print(basic_type b_type);
uint64_t basic_type_get_size(basic_type b_type);

uint64_t symbol_type_array_get_size(p_symbol_type_array p_array);