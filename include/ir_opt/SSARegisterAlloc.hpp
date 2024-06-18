#pragma once

#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <ir_opt/Manager.hpp>
#include <set>
#include <queue>
#include <ir_opt/RegisterAlloc.hpp>
#include <util/hash_pair.hpp>

class SSARegisterAlloc
{
    int K_R, K_S;
    LivenessAnalysis LA;
    int vregNum;
    std::unordered_map<int, int> color;
    std::unordered_set<int> spilledNodes;
    void build(Function *p_func);
    std::vector<std::vector<int>> G;
    std::unordered_set<std::pair<int, int>, pair_hash> AdjSet;

public:
    void run(Function *p_func);
    void Spill(Function *p_func);
    void SpillBB_R(BasicBlock *bb);
    void SpillBB_S(BasicBlock *bb);
    void RewriteProgram(Function *p_func);
    void MakeGraph(Function *p_func);
    void AddEdge(int x, int y);
    void AssignColor_R(Function *p_func);
    void AssignColor_S(Function *p_func);
};