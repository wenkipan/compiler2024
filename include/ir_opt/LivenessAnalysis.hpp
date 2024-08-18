#pragma once
#include <unordered_set>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/BitVector.hpp"
#include "../../include/ir_opt/DCE.hpp"

const int K_R = 13, K_S = 32, K_Q = 8, Para_R = 4, Para_S = 16, Para_Sum = 20;
class LivenessAnalysis
{
    const int INF = 1000000000;
    Function *parent;
    std::unordered_map<Value *, int> ValueIdMap;
    std::unordered_map<BasicBlock *, bit_vector> InSet;
    std::unordered_map<BasicBlock *, bit_vector> OutSet;
    std::unordered_map<BasicBlock *, bit_vector> DefSet;
    std::unordered_map<BasicBlock *, bit_vector> UseSet;
    std::unordered_map<BasicBlock *, std::vector<int>> InDis;
    std::unordered_map<BasicBlock *, std::vector<int>> OutDis;
    std::vector<BasicBlock *> BBs;
    std::vector<Value *> Vals;
    std::vector<std::unordered_set<int>> valSet;
    std::vector<int> Father;
    std::vector<int> is_float, is_vector, is_int;
    std::unordered_set<BasicBlock *> vis;
    void init();
    bool work_BB(BasicBlock *bb);
    void DefAndUseAnalysis();
    void dfs(BasicBlock *u);
    friend class RegisterAlloc;
    friend class SSARegisterAlloc;

public:
    void run(Function *func);
    int get_father(int x);
    void combine(int x, int y);
};