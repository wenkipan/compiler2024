#pragma once

#include <ir/Value.hpp>
#include <ir/Edge.hpp>

class User : public Value
{

public:
    User(TypeEnum basic_type);
    User(p_symbol_var p_var);
    // User(p_symbol_var p_var, basic_type basic);
    User(p_symbol_func p_func);
    User(Type *p_array, bool _ele);
    virtual ~User();

    //virtual void print() { assert(0); }
};
