#pragma once

#include <ir/Function.hpp>
#include <ir/GlobalValue.hpp>
#include <ir/GlobalVariable.hpp>
#include <program/program.hpp>

// class loop
// {
//     std::vector<Function *> *functions;
// };

class Module
{
    std::string infile;
    std::string outfile;
    std::vector<GlobalVariable *> *global_variables;
    std::vector<Function *> *functions;

public:
    Module(const std::string &input, const std::string &output);
    ~Module();

    void func_push_back(Function *p_func);
    void Gvar_add(p_symbol_var p_var);

    void lowerIR();

    std::vector<Function *> *get_funcs();

    void GenerSet();

    void llvm_print();
    void print();
};