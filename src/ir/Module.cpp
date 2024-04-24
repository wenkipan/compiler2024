#include <ir/Module.hpp>
#include <ir/IRGener.hpp>
#include <iostream>

Module::Module(const std::string &input, const std::string &output)
    : infile(input), outfile(output),
      global_variables(new std::vector<GlobalVariable *>),
      functions(new std::vector<Function *>)
{
}

Module::~Module()
{
    // std::cout << global_variables->size() << std::endl;
    for (GlobalValue *p_Gvar : (*global_variables))
        delete p_Gvar;
    for (GlobalValue *p_func : (*functions))
        delete p_func;
    delete global_variables;
    delete functions;
}

void Module::GenerSet()
{
    GenFunction::module = this;
}

void Module::Gvar_add(p_symbol_var p_var)
{
    // printf("name:  %s\n", p_var->name);
    if (!p_var->is_global)
        return;
    GlobalVariable *p_value = new GlobalVariable(p_var);
    GenFunction::_global_map->insert(std::pair<p_symbol_var, Value *>(p_var, p_value));
    global_variables->emplace_back(p_value);
}

void Module::func_push_back(Function *p_func)
{

    functions->emplace_back(p_func);
}

std::vector<Function *> *Module::get_funcs()
{
    return functions;
}

void Module::print()
{
    printf("\n      ---------------IR begin---------------------\n");
    for (GlobalVariable *p_var : (*global_variables))
        p_var->print();
    for (Function *p_func : (*functions))
        p_func->print();
    printf("\n      ---------------IR end-----------------------\n");
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