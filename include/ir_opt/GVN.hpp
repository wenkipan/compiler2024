#pragma once

#include "../../include/ir/ir.hpp"
class GVN
{
public:
    int is_debug = 0;
    Function *f;
    std::unordered_map<std::string, Instrution *> hashmap;
    std::unordered_map<Function *, bool> sideeffect_func;
    // void init();
    void run(Function *func);
    Instrution *check_hash(Instrution *instr);
    std::string makehashtag(Instrution *i);
    std::string makereversehashtag(Instrution *i);
    bool canexchangeop(Instrution *instr);
    void visit_instr(Instrution *instr);
    void visit_phi(PHINode *phi);
};