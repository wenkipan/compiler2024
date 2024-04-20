#pragma once
#include <util/util.hpp>
#include <symbol/symbol.hpp>
#include <symbol/func.hpp>
#include <symbol/var.hpp>
#include <symbol/str.hpp>
typedef class program *p_program;
class program
{
public:
    list_head variable;
    list_head function;
    list_head string;

    uint64_t variable_cnt;
    uint64_t function_cnt;

    char *input;
    char *output;
    // p_program program_gen(const char *input, const char *output);
    program(const char *input, const char *output);
    ~program();
    bool program_add_str(p_symbol_str p_str);
    bool program_add_global(p_symbol_var p_var);
    bool program_add_function(p_symbol_func p_func);

    void program_drop(p_program p_program);

    void program_del_global(p_program p_program, p_symbol_var p_var);
    void program_global_set_id(p_program p_program);
    void program_variable_print(p_program p_program);
    void program_ir_print(p_program p_program);
    void program_ir_dom_info_print(p_program p_program);

    // print
    void program_variable_print();
};