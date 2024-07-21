#pragma once
#include <string>

#include "../../include/ir/GlobalValue.hpp"
#include "../../include/ir/Constant.hpp"

class GlobalVariable : public GlobalValue
{
    bool is_init;
    bool is_const;
    Constant *init;

public:
    GlobalVariable(p_symbol_var p_var);
    GlobalVariable(Type *type, std::string name, std::vector<int> vi, bool isconst);
    GlobalVariable(Type *type, std::string name, std::vector<float> vi, bool isconst);

    ~GlobalVariable() override;

    bool check_const() { return is_const; }
    Constant *get_init() { return init; }

    void print() override;
    void llvm_print();
};