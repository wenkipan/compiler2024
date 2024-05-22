#pragma once

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <ir_opt/BitVector.hpp>
#include <unordered_set>

class LivenessAnalysis
{
    Function *parent;
    std::unordered_map<Value *, int> ValueIdMap;
    std::unordered_map<BasicBlock *, bit_vector> InSet;
    std::unordered_map<BasicBlock *, bit_vector> OutSet;
    std::unordered_map<BasicBlock *, bit_vector> DefSet;
    std::unordered_map<BasicBlock *, bit_vector> UseSet;
    std::vector<BasicBlock *> BBs;
    std::vector<Value *> Vals;
    std::unordered_set<BasicBlock *> vis;
    void init();
    bool work_BB(BasicBlock *bb);
    void DefAndUseAnalysis();
    void dfs(BasicBlock *u);

public:
    void run(Function *func);
};