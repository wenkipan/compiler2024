#pragma once
#include "../../include/ir/ir.hpp"

class LargeToGlobal
{
public:
    void PassRun(Module *m)
    {
        run(m);
    }
    void run(Module *m);
};
