#pragma once

#include <ir_opt/SCEV.hpp>
#include <ir_opt/IRCopy.hpp>

class loopFullunroll
{

    static std::set<BasicBlock *> SubMap;

    SCEV *_SCEV;
    Loop_Analysis *_Loop;

    int codesize;

public:
    loopFullunroll();
    ~loopFullunroll();

    void SetVal(PHINode *phi, IRCopy &copyer, Loop *loop);
    void SetVal(Instrution *instr, IRCopy &copyer, Loop *loop);
    void SetUser(Instrution *instr, IRCopy &copyer, Loop *loop);

    bool Finder(Loop *loop, Function *func);
    void FullUnroll(Loop *loop, int times, Function *p_func);

    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};