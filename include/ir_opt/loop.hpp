#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/SCEV.hpp"
#include "../../include/ir_opt/IRCopy.hpp"

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

class loopFFF
{
    SCEV *_SCEV;
    Loop_Analysis *_Loop;

    const int times = 4;

public:
    loopFFF();
    ~loopFFF();

    void SetVal(PHINode *phi, IRCopy &copyer, Loop *loop);
    void SetVal(Instrution *instr, IRCopy &copyer, Loop *loop);

    bool analysis(Loop *loop);
    BasicBlock *earlyBB(Loop *loop);

    void Unroll(Loop *loop);
    void searchFunc(Loop *loop, Function *func);
    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};