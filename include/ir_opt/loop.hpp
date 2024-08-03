#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/SCEV.hpp"

class loopYYY
{
    SCEV *scev;
    Loop_Analysis *LA;
    Value *p_set;

public:
    loopYYY();
    ~loopYYY();

    void W1(Loop *loop);
    void W2(Loop *loop);
    void W3(Loop *loop);

    void FuncAnalysis(Function *func);
    void PassRun(Module *p_module);
};
