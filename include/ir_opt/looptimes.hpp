#pragma once

#include <ir_opt/SCEV.hpp>

class looptimes
{

    static std::set<BasicBlock *> SubMap;

    SCEV *_SCEV;
    Loop_Analysis *_Loop;

    int codesize;

public:
    looptimes();
    ~looptimes();

    bool isSub(Loop *loop);
    bool Finder(Loop *loop, Function *func);
    BasicBlock *Unroll(Loop *loop, const int times, Function *p_func);
    void FullUnroll(Loop *loop, int times, Function *p_func);

    void FuncDealer(Function *p_func);
    void PassRun(Module *p_module);
};