#pragma once

#include "../../include/ir/ir.hpp"

class GAD
{
    std::vector<Instrution *> Dels;

public:
    bool check(Value *GV);
    void PassRun(Module *p_module);
};