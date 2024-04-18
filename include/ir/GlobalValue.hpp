
#pragma once
#include <ir/User.hpp>

class GlobalValue : public User
{
    std::string name;

public:
    GlobalValue(p_symbol_var p_var);
    GlobalValue(p_symbol_func p_func);
    std::string get_name();
};