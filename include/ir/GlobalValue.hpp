
#pragma once
#include <ir/User.hpp>

class GlobalValue : public User
{
    std::string name;

public:
    GlobalValue(p_symbol_var p_var);
    GlobalValue(p_symbol_func p_func);
    ~GlobalValue() override;

    std::string get_name();
};