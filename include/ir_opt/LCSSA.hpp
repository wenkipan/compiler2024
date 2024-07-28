#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Loop.hpp"
#include "../../include/ir_opt/DomTree.hpp"

class LCSSA
{
    Function *p_func;
    Loop_Analysis *p_LA;

public:
    LCSSA(Function *_func, Loop_Analysis *_LA)
        : p_func(_func), p_LA(_LA) {}

    void loopdealer(Loop *loop, DomTree &domtree);
    void run();
};