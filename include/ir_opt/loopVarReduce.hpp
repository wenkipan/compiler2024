#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/SCEV.hpp"
#include "../../include/ir_opt/DomTree.hpp"

class loopVarReduce
{
    SCEV *_SCEV = nullptr;
    Loop_Analysis *_Loop = nullptr;

public:
    loopVarReduce();
    ~loopVarReduce();

    void Reduce(Loop *loop, DomTree &_domtree);
    void PassRun(Module *_module);
};