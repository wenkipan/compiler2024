#pragma once

#include <util/util.hpp>
#include <frontend/syntax_type_array.hpp>
#include <ast/ast.hpp>
typedef class syntax_init *p_syntax_init;
class syntax_init
{
public:
    bool is_exp;
    bool syntax_const;
    union
    {
        p_ast_exp p_exp;
        list_head list;
    };

    list_head node;
    // p_syntax_init syntax_init_list_gen(void);
    syntax_init();
    // p_syntax_init syntax_init_exp_gen(p_ast_exp p_exp);
    syntax_init(p_ast_exp p_exp);
    // void syntax_init_drop(p_syntax_init p_init);
    ~syntax_init();

    p_syntax_init syntax_init_list_add(p_syntax_init p_init);
    p_syntax_init syntax_init_regular(p_symbol_type p_type);

    p_list_head syntax_init_get_head();
    p_ast_exp syntax_init_get_exp();

    p_ast_exp syntax_init_find_exp(p_symbol_type p_type, size_t offset);

    p_ast_stmt syntax_return(p_ast_exp p_exp);
};
p_syntax_init syntax_init_get_entry(p_list_head p_node);