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
    std::vector<GlobalVariable *> *global_variables;
    std::vector<Function *> *functions;
    std::string infile;
    std::string outfile;

public:
    Module(const std::string &input, const std::string &output);
    Module(const std::string &input, const std::string &output,
           p_program p_program);

    void func_push_back(Function *p_func);

    std::vector<Function *> *get_funcs();
};