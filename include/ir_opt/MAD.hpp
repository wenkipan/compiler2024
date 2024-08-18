#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/Loop.hpp"

class MAD
{
    Function *func = nullptr;

    std::vector<Instrution *> nwDels;

    std::unordered_map<BasicBlock *, bool> BBmap_;

    bool _findST(GEP *gep, Function *func, int cnt, Call *&_call);

    Loop_Analysis *Loop_;

public:
    MAD();
    ~MAD();

    void LDD();
    void SSD();
    void UAD();
    void ARD();
    void SMO();

    void BBFilter(Loop *loop);

    void FuncDealer();
    void PassRun(Module *p_module);
};