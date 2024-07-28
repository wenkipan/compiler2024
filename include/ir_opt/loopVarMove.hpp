#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/SCEV.hpp"
#include "../../include/ir_opt/DomTree.hpp"

class loopVarMove
{
    SCEV *_scev = nullptr;
    Loop_Analysis *_Loop = nullptr;

public:
    loopVarMove();
    ~loopVarMove();

    void VarMove(Loop *loop, DomTree &_domtree);
    void PassRun(Module *p_module);
};