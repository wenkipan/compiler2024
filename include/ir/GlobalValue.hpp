
#pragma once
#include <ir/User.hpp>

class GlobalValue : public User
{
    std::string name;

public:
    GlobalValue();
    GlobalValue(p_symbol_var p_var);
    GlobalValue(p_symbol_func p_func);
    GlobalValue(std::string _name);
    ~GlobalValue() override;

    std::string get_name();
};