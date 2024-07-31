#pragma once
#include "../../../include/backend/arm/arm.hpp"

class BlockMerge
{

public:
    void run(ArmModule *m);
    void single_br(ArmModule *m);
};