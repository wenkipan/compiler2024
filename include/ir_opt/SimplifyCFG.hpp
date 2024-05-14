#pragma once
#include <ir/ir.hpp>
#include <unordered_set>
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
    void PassRun(Module *m)
    {
        for (auto func : *m->get_funcs())
        {
            if (func->get_isExternal())
                continue;
            SimplifyCFG s;
            s.run(func);
        }
    }
};