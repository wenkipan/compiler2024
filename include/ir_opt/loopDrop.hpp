#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Loop.hpp"
class LoopDrop // need sccp
{
    Loop_Analysis *loop = nullptr;

public:
    LoopDrop();
    ~LoopDrop();

    void DealLoop(Loop *loop);
    void PassRun(Module *p_module);
};