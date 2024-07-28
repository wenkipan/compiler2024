#include "../../include/ir/ir.hpp"

class modTofunc
{

public:
    void PassRun(Module *m)
    {
        run(m);
    }
    void run(Module *m);
};