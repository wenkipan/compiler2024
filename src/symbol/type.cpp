
#include <symbol/type.hpp>
uint64_t basic_type_get_size(basic_type b_type)
{
    return 4;
}
// uint64_t symbol_type_get_size(p_symbol_type p_type)
// {
//     return p_type->size;
// }
uint64_t symbol_type_array_get_size(p_symbol_type_array p_array)
{
    assert(p_array);
    return p_array->size;
}

symbol_type::symbol_type(basic_type basic)
    : ref_level(0),
      // array(list_init_head(&this->array)),
      basic(basic),
      size(1)
{
    array = list_init_head(&this->array);
}

void symbol_type::symbol_type_push_ptr()
{
    ++(this->ref_level);
}
void symbol_type::symbol_type_pop_ptr()
{
    --(this->ref_level);
}
// maybe problem
p_symbol_type_array symbol_type::symbol_type_top_array()
{
    p_symbol_type_array p_array = list_entry(this->array.p_next, symbol_type_array, node);
    return p_array;
}
p_symbol_type_array symbol_type::symbol_type_pop_array()
{
    assert(this->ref_level == 0);
    p_symbol_type_array p_array = list_entry(this->array.p_next, symbol_type_array, node);
    list_del(&p_array->node);
    this->size /= p_array->size;
    return p_array;
}
p_symbol_type symbol_type::symbol_type_copy()
{
    // maybe problem
    p_symbol_type p_copy = new symbol_type(this->basic);
    p_list_head p_node;
    list_for_each_tail(p_node, &this->array)
    {
        p_symbol_type_array p_array = list_entry(p_node, symbol_type_array, node);
        p_symbol_type_array p_array_copy = new symbol_type_array(p_array);
        p_array_copy->symbol_type_push_array(p_copy);
    }
    p_copy->ref_level = this->ref_level;

    return p_copy;
}
symbol_type::~symbol_type()
{
    assert(this);
    p_list_head p_node, p_next;
    list_for_each_safe(p_node, p_next, &this->array)
    {
        p_symbol_type_array p_array = list_entry(p_node, symbol_type_array, node);
        list_del(&p_array->node);
        delete (p_array);
    }
}

symbol_type_array::symbol_type_array(size_t size)
    : size(size),
      node(list_init_head(&this->node))
{
}
symbol_type_array::symbol_type_array(p_symbol_type_array p_array)
    : size(p_array->size),
      node(list_init_head(&this->node))
{
    // this = symbol_type_array(p_array->size);
}
void symbol_type_array::symbol_type_push_array(p_symbol_type p_type_arr)
{
    list_add_next(&this->node, &p_type_arr->array);
    p_type_arr->size *= this->size;
}
void symbol_type::symbol_type_print()
{
    p_symbol_type p_type = this;
    assert(p_type);
    uint64_t arr_level = 0;
    p_list_head p_node;
    list_for_each(p_node, &p_type->array)
    {
        p_symbol_type_array p_array = list_entry(p_node, symbol_type_array, node);
        printf("[%ld X ", symbol_type_array_get_size(p_array));
        ++arr_level;
    }
    assert(p_type->basic != type_void || (list_head_alone(&p_type->array) && p_type->ref_level > 0));
    symbol_basic_type_print(p_type->basic);
    for (uint64_t i = 0; i < arr_level; ++i)
    {
        printf("]");
    }
    for (uint64_t i = 0; i < p_type->ref_level; ++i)
    {
        printf("*");
    }
}

void symbol_basic_type_print(basic_type b_type)
{
    if (b_type == type_void)
        printf("void");
    else if (b_type == type_i32)
        printf("i32");
    else if (b_type == type_f32)
        printf("f32");
    else if (b_type == type_str)
        printf("str");
}
