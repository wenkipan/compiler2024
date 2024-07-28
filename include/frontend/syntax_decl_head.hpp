#pragma once
#include "../../include/symbol/symbol.hpp"

typedef class syntax_decl_head *p_syntax_decl_head;
class syntax_decl_head
{
public:
    bool is_const;
    basic_type type;
    // p_syntax_decl_head syntax_decl_head_gen(basic_type b_type, bool is_const);
    syntax_decl_head(basic_type b_type, bool is_const);
    // nothing to do
    // void syntax_decl_head_drop(p_syntax_decl_head p_decl_list);

    // bool syntax_decl_head_get_is_const(p_syntax_decl_head p_list);
    // basic_type syntax_decl_head_get_basic(p_syntax_decl_head p_list);
};
