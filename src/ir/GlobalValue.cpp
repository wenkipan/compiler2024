#include <ir/GlobalValue.hpp>
#include <string.h>

GlobalValue::GlobalValue(p_symbol_var p_var)
    : User(p_var)
{
    name = std::string(p_var->name, strlen(p_var->name));
}

GlobalValue::GlobalValue(p_symbol_func p_func)
    : User(p_func)
{
    name = std::string(p_func->name, strlen(p_func->name));
}

std::string GlobalValue::get_name()
{
    return name;
}