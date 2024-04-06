#include <program/program.hpp>

program::program(const char *input, const char *output)
    : variable(list_head_init(&this->variable)),
      function(list_head_init(&this->function)),
      string(list_head_init(&this->string)),
      variable_cnt(0),
      function_cnt(0),
      input(NULL),
      output(NULL)
{
    if (!output)
    {
        char *tmp = new char[9];
        strcpy(tmp, "output.s");
        this->output = tmp;
    }
    else
    {
        char *tmp = new char[strlen(output) + 1];
        strcpy(tmp, output);
        this->output = tmp;
    }
    if (!input)
    {
        char *tmp = new char[10];
        strcpy(tmp, "std input");
        this->input = tmp;
    }
    else
    {
        char *tmp = new char[strlen(input) + 1];
        strcpy(tmp, input);
        this->input = tmp;
    }
}
bool program::program_add_str(p_symbol_str p_str)
{
    return list_add_prev(&p_str->node, &this->string);
}
bool program::program_add_global(p_symbol_var p_var)
{
    p_var->id = this->variable_cnt++;
    return list_add_prev(&p_var->node, &this->variable);
}
bool program::program_add_function(p_symbol_func p_func)
{
    p_func->id = this->function_cnt++;
    return list_add_prev(&p_func->node, &this->function);
}

void program::program_variable_print()
{
    p_program p_program = this;
    assert(p_program);
    printf("=== program start ===\n");
    p_list_head p_node;
    list_for_each(p_node, &p_program->variable)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        printf("global ");
        if (p_var->is_const)
            printf("constant ");
        p_var->symbol_init_print();
    }
    list_for_each(p_node, &p_program->string)
    {
        p_symbol_str p_str = list_entry(p_node, symbol_str, node);
        printf("string ");
        p_str->symbol_str_print();
    }
    list_for_each(p_node, &p_program->function)
    {
        p_symbol_func p_func = list_entry(p_node, symbol_func, node);
        p_func->symbol_func_init_print();
    }
    printf("=== program end ===\n");
}