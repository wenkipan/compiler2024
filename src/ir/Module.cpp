#include <ir/Module.hpp>
#include <ir/IRGener.hpp>

Module::Module(const std::string &input, const std::string &output)
    : infile(input), outfile(output)
{
    GenFunction::module = this;
}

Module::Module(const std::string &input, const std::string &output, p_program p_program)
    : infile(input), outfile(output)
{
    GenFunction::module = this;
    p_list_head p_node;
    list_for_each(p_node, &p_program->variable)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        GlobalVariable *p_value = new GlobalVariable(p_var);
        GenFunction::_global_map->insert(std::pair<p_symbol_var, Value *>(p_var, p_value));
        global_variables->emplace_back(p_value);
    }
    /*
    p_list_head p_node;
    list_for_each(p_node, &p_program->variable)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        GlobalVariable *p_value = new GlobalVariable(p_var);
        global_variables->emplace_back(p_value);
    }

    list_for_each(p_node, &p_program->function)
    {
        p_symbol_func p_func = list_entry(p_node, symbol_func, node);
        Function *p_function = new Function(p_func);
        functions->emplace_back(p_function);
    }
    */
}

void Module::func_push_back(Function *p_func)
{

    functions->emplace_back(p_func);
}

std::vector<Function *> *Module::get_funcs()
{
    return functions;
}