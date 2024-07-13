#pragma once

#include <ir_opt/SCEV.hpp>
#include <ir_opt/IRCopy.hpp>
#include <ir_opt/LCSSA.hpp>

class loopunswitch
{
    int codesize;
    SCEV *_SCEV;
    Loop_Analysis *_Loop;

public:
    loopunswitch();
    ~loopunswitch();

    BasicBlock *checkIF(Loop *loop);
    void SetVal(PHINode *phi, IRCopy &copyer, Loop *loop);
    void SetVal(Instrution *instr, IRCopy &copyer, Loop *loop);
    void SetUser(Instrution *instr, IRCopy &copyer, Loop *loop);
    void SetPrevBB(BasicBlock *workBB, Loop *loop, IRCopy &copyer);

    void Unswitch(Loop *loop, Function *p_func, BasicBlock *workBB, DomTree &domtree);
    void searchFunc(Loop *loop, Function *func, DomTree &domtree, bool &flag);
    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};