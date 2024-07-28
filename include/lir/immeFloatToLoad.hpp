#pragma once

#include "../../include/ir/ir.hpp"

class immeFloatToLoad
{
private:
    /* data */
public:
    void run(Module *m);
    void PassRun(Module *m)
    {
        run(m);
    }
};
