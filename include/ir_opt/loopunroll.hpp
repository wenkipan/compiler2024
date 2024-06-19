#pragma once

#include <ir_opt/SCEV.hpp>

class loopunroll
{
    SCEV *_SCEV;
    Loop_Analysis *_Loop;

public:
    loopunroll();
    ~loopunroll();

    void Unroll(Loop *loop, int times, Function *p_func);
    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};