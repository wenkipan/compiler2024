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
    std::vector<Functon *> *functions;
    std::string infile;
    std::string outfile;

public:
    Module();
};