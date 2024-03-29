#pragma once

#include <symbol.h>
#include <syntax_type_array.h>
#include <syntax_init.h>
typedef class syntax_decl *p_syntax_decl;
class syntax_decl
{
    char *name;
    p_syntax_type_array p_array;
    p_syntax_init p_init;

    p_syntax_decl syntax_decl_gen(char *name);
    void syntax_decl_drop(p_syntax_decl p_decl);

    const char *syntax_decl_list_node_get_name(p_syntax_decl p_decl);
    p_syntax_type_array syntax_decl_list_node_get_array(p_syntax_decl p_decl);
    p_syntax_init syntax_decl_list_node_get_init(p_syntax_decl p_decl);
    p_syntax_decl syntax_decl_arr(p_syntax_decl p_decl, p_ast_exp p_exp);
    p_syntax_decl syntax_decl_init(p_syntax_decl p_decl, p_syntax_init p_init);
};