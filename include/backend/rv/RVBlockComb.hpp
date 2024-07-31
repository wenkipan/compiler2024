#pragma once

#include "../../../include/backend/rv/RV.hpp"

class RVBlockComb
{
public:
    void run(RVFunc *f);
    void run(RVModule *m);
    void merge_sigle_j(RVFunc *f);
};