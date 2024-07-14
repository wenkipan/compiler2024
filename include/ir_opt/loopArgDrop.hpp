#pragma once

#include <ir/ir.hpp>
#include <ir_opt/Loop.hpp>
#include <ir_opt/SCEV.hpp>
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