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
    int K;
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
    void SpillBB(BasicBlock *bb);
    void RewriteProgram(Function *p_func);
    void MakeGraph(Function *p_func);
    void AddEdge(int x, int y);
    void AssignColor(Function *p_func);
};