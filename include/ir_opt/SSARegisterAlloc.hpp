#pragma once

#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <ir_opt/Manager.hpp>
#include <set>
#include <queue>
#include <util/hash_pair.hpp>

class SSARegisterAlloc
{
    LivenessAnalysis LA;
    int vregNum;
    std::unordered_map<int, int> color;
    std::unordered_set<int> spilledNodes;
    std::vector<std::vector<int>> G;
    std::unordered_set<std::pair<int, int>, pair_hash> AdjSet;
    std::unordered_map<Alloca *, Param *> paraMap;
    std::unordered_map<Call *, std::vector<int>> callLiveVreg;
    void Spill(Function *p_func);
    void SpillBB_R(BasicBlock *bb);
    void SpillBB_S(BasicBlock *bb);
    void spillUserPhi(int x);
    void RewriteProgram(Function *p_func);
    void MakeGraph(Function *p_func);
    void AddEdge(int x, int y);
    void AssignColor_R(Function *p_func);
    void AssignColor_S(Function *p_func);
    void AddBB(Function *p_func);

public:
    void run(Function *p_func);
    int getReg(Value *val);
    Param *whichPara(Alloca *alloc);
    std::vector<int> regsStillAliveAfterCall(Call *call);
};