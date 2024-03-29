#pragma once
#include <util.h>
#include <syntax_type_array.h>
#include <ast.h>
typedef class syntax_init *p_syntax_init;
class syntax_init
{
    bool is_exp;
    bool syntax_const;
    union
    {
        p_ast_exp p_exp;
        list_head list;
    };

    list_head node;
    p_syntax_init syntax_init_list_gen(void);
    p_syntax_init syntax_init_exp_gen(p_ast_exp p_exp);
    p_syntax_init syntax_init_list_add(p_syntax_init p_list, p_syntax_init p_init);

    p_syntax_init syntax_init_regular(p_syntax_init p_init, p_symbol_type p_type);
    void syntax_init_drop(p_syntax_init p_init);

    p_syntax_init syntax_init_get_entry(p_list_head p_node);
    p_list_head syntax_init_get_head(p_syntax_init p_init);
    p_ast_exp syntax_init_get_exp(p_syntax_init p_init);

    p_ast_exp syntax_init_find_exp(p_syntax_init p_init, p_symbol_type p_type, size_t offset);
};