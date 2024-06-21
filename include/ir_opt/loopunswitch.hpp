#pragma once

#include <ir_opt/SCEV.hpp>
#include <ir_opt/LCSSA.hpp>

class loopunswitch
{
    SCEV *_SCEV;
    Loop_Analysis *_Loop;

public:
    loopunswitch();
    ~loopunswitch();

    void Unswitch(Loop *loop, Function *p_func, BasicBlock *workBB, DomTree &domtree);
    void searchFunc(Loop *loop, Function *func, DomTree &domtree, bool &flag);
    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};