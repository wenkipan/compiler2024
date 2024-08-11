#pragma once
#include "../../include/ir/ir.hpp"

class SideEffect
{
public:
    bool check_mem(Function *f);
    bool check_recursive(Function *f);
    bool run(Function *f);
};
