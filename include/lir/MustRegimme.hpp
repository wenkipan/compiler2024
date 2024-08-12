#pragma once

#include "../../include/ir/ir.hpp"

class MustRegimme
{

public:
    void PassRun(Module *m)
    {
        run(m);
    }
    void run(Module *m);
};
