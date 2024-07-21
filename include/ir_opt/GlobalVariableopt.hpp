#pragma once
#include <unordered_set>

#include "../../include/ir/ir.hpp"

class GlobalVariableopt
{
    Module *m;
    std::unordered_set<GlobalVariable *> usefulGV;

public:
    GlobalVariableopt() {}
    ~GlobalVariableopt() {}
    void run();
    void mark_live();
    void replace_with_constant(); // no store at end of the def-use
    bool dfs_GEP(Instrution *i);
    void demote_to_local(); // only if main func
    void PassRun(Module *module);
};
