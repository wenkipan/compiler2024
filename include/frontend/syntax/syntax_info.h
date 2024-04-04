#pragma once
#include <symbol_table.h>
#include <syntax_decl_head.h>
#include <syntax_decl.h>
#include <program.h>
#include <ast.h>
typedef class syntax_info *p_syntax_info;
class syntax_info
{
public:
    p_symbol_table p_table;
    p_program _p_program;
    p_symbol_func p_func;
    p_ast_block p_block;
    p_symbol_func p_mem_set;

    // p_syntax_info syntax_info_gen(const char *input, const char *output);
    syntax_info(const char *input, const char *output);
    // void syntax_info_drop(p_syntax_info p_info);
    ~syntax_info();

    // p_program syntax_info_get_program(p_syntax_info p_info);

    void syntax_set_block(p_ast_block p_block);

    void syntax_zone_push();
    void syntax_zone_pop();
    p_symbol_var syntax_find_var(const char *name);
    p_symbol_func syntax_find_func(const char *name);
    p_symbol_str syntax_get_str(const char *string);

    void syntax_set_func(p_syntax_info p_info, p_symbol_func p_func);
    void syntax_func_add_variable(p_symbol_var p_var);
    void syntax_func_add_param(p_symbol_var p_var);

    void syntax_program_add_variable(p_symbol_var p_var);
    void syntax_program_add_function(p_symbol_func p_func);

    p_syntax_decl_head syntax_declaration(p_syntax_decl_head p_head, p_syntax_decl p_decl);

    void syntax_func_head(p_syntax_info p_info, basic_type type, char *name);
    void syntax_func_end(p_syntax_info p_info, p_ast_block p_block);

    void syntax_rtlib_func_init(p_syntax_info p_info);
};