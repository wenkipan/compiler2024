#pragma once

#include <ir/GlobalValue.hpp>
#include <ir/GlobalVariable.hpp>
#include <ir/Function.hpp>

// class loop
// {
//     std::vector<Functon *> *functions;
// };

class Module
{
    std::vector<GlobalVariable *> *global_variables;
    std::vector<Function *> *functions;
    std::string infile;
    std::string outfile;

public:
    Module();
    void functions_push_back(Function *f) { functions->push_back(f); }
    std::vector<GlobalVariable *> *get_global_variables() { return global_variables; }
    std::vector<Function *> *get_functions() { return functions; }
};