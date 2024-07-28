#pragma once
#include <unordered_set>

#include "../../include/ir/ir.hpp"

class SimplifyCFG
{
    Function *function;
    std::queue<PHINode *> philist;

public:
    // SimplifyCFG(Function *f) { function = f; }
    bool run(Function *f);
    void elimate_no_predesessor_block();
    bool merge_single_predecessor_block();
    void eliminate_single_br_block();
    void eliminate_single_predecessor_phi();
};