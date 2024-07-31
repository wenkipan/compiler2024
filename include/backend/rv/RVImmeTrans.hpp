#pragma once
#include "../../../include/backend/rv/RV.hpp"

class RVImmeTrans
{
public:
    void run(RVModule *m);
    void deal_immef(RVFunc *f);
    void deal_large_off(RVFunc *f);
};