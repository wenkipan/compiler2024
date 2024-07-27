#pragma once

#include <ir/ir.hpp>

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
    void must_2_reg(Module *m);
    void large_imme(Module *m);
};
