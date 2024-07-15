#pragma once

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <ir_opt/BitVector.hpp>
#include <unordered_set>

const int K_R = 12, K_S = 32, Para_R = 4, Para_S = 16, Para_Sum = 20;
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
    std::vector<int> is_float;
    std::unordered_set<BasicBlock *> vis;
    void init();
    bool work_BB(BasicBlock *bb);
    void DefAndUseAnalysis();
    void dfs(BasicBlock *u);
    friend class RegisterAlloc;
    friend class SSARegisterAlloc;

public:
    void run(Function *func);
};