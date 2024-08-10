#include "../../../include/backend/arm/arm.hpp"

class DeadALU
{
public:
    void run(ArmFunc *f);
    void run(ArmModule *m)
    {
        for (auto f : m->get_funcs())
        {
            if (f->is_external())
                continue;
            run(f);
        }
    }
};
