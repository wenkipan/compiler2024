#include <backend/rv/RV.hpp>
RVInstr::RVInstr(RVENUM i, RVcond c, RVfloat f)
    : instr_enum(i), cond_enum(c)
{
    float_enums.push_back(f);
}
RVInstr::RVInstr(RVENUM i, RVcond c, RVfloat f1, RVfloat f2)
    : instr_enum(i), cond_enum(c)
{
    float_enums.push_back(f1);
    float_enums.push_back(f2);
}
