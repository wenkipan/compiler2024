#include <frontend/syntax_type_array.hpp>

syntax_type_array::syntax_type_array(uint64_t size)
    : size(size),
      p_prev(NULL)
{
}

p_symbol_type syntax_type_array::syntax_type_trans(basic_type b_type)
{
    p_syntax_type_array p_array = this;
    p_symbol_type p_type = new symbol_type(b_type);
    while (p_array)
    {
        p_syntax_type_array p_del = p_array;
        p_array = p_array->p_prev;
        if (p_del->size)
        {
            // printf("%lu\n", p_del->size);
            p_symbol_type_array p_add = new symbol_type_array(p_del->size);
            p_add->symbol_type_push_array(p_type);
        }
        else
        {
            p_type->symbol_type_push_ptr();
        }
        delete (p_del);
    }
    return p_type;
}
p_syntax_type_array syntax_type_array::syntax_type_add_array(p_syntax_type_array p_new_head)
{
    p_new_head->p_prev = this;
    return p_new_head;
}