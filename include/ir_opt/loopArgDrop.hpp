#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Loop.hpp"
#include "../../include/ir_opt/SCEV.hpp"

class LoopArgDrop // need sccp
{
    SCEV *scev = nullptr;
    Loop_Analysis *LA = nullptr;

public:
    LoopArgDrop();
    ~LoopArgDrop();

    void DealLoop(Loop *loop);
    void PassRun(Module *p_module);
};