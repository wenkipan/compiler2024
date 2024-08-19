#pragma once

#include "../../include/ir_opt/SCEV.hpp"
#include "../../include/ir_opt/IRCopy.hpp"
#include "../../include/ir_opt/LCSSA.hpp"

class loopUnroll
{

    SCEV *_SCEV;
    Loop_Analysis *_Loop;

    int codesize;
    bool flag = false;

    const int times = 4;

    static std::set<BasicBlock *> *BBmap;

public:
    loopUnroll();
    ~loopUnroll();

    void SetVal(PHINode *phi, IRCopy &copyer, Loop *loop);
    void SetVal(Instrution *instr, IRCopy &copyer, Loop *loop);
    void SetUser(Instrution *instr, IRCopy &copyer, Loop *loop);

    bool analysis(Loop *loop);

    void Unroll(Loop *loop);
    void searchFunc(Loop *loop, Function *func);
    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};
