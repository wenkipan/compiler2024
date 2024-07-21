#pragma once

#include "../../include/ir/ir.hpp"

class immeIntTomove
{
private:
    /* data */
public:
    void PassRun(Module *m)
    {
        run(m);
    }
    void run(Module *m);
};
