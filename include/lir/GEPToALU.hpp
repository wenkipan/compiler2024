#pragma once

#include "../../include/ir/ir.hpp"

class GEPToALU
{

public:
    void run(Function *f);
    void combine_gep(Function *f);
};
