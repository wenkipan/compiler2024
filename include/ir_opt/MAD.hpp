#pragma once

#include <ir/ir.hpp>

class MAD
{
    Function *func = nullptr;

    std::vector<Instrution *> nwDels;

    bool _findST(GEP *gep, Function *func, int cnt, Call *&_call);

public:
    void LDD();
    void SSD();
    void UAD();
    void ARD();

    void FuncDealer();
    void PassRun(Module *p_module);
};