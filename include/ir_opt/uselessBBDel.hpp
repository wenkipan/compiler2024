#pragma once

#include <ir/ir.hpp>

class uselessBBDel
{
    Function *function;

public:
    bool run(Function *f);
    void eliminate_uselessBB();
};