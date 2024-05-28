#pragma once

#include <ir/GlobalValue.hpp>
#include <ir/Constant.hpp>
#include <string>

class GlobalVariable : public GlobalValue
{
    bool is_init;
    bool is_const;
    Constant *init;

public:
    GlobalVariable(p_symbol_var p_var);
    ~GlobalVariable() override;

    bool check_const() { return is_const; }
    Constant *get_init() { return init; }

    void print() override;
    void llvm_print();
};