#pragma once

#include <symbol.h>
typedef struct syntax_type_array *p_syntax_type_array;
class syntax_type_array
{
public:
    uint64_t size;
    p_syntax_type_array p_prev;

    // p_syntax_type_array syntax_type_array_gen(uint64_t size);
    syntax_type_array(uint64_t size);

    p_symbol_type syntax_type_trans(basic_type b_type);
    p_syntax_type_array syntax_type_add_array(p_syntax_type_array p_new_head);
};