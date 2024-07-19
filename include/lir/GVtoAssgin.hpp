#pragma once
#include <ir_opt/DomTree.hpp>
#include <ir/ir.hpp>

class GVtoA
{
    std::unordered_map<GlobalVariable *, std::vector<Edge *>> gvused_edge;
    DomTree *domtree;

public:
    void run(Function *f);
    void add_useredge(GlobalVariable *, Edge *e);
    BasicBlock *find_LCA(std::vector<Edge *> users);
    BasicBlock *find_LCA(BasicBlock *a, BasicBlock *b);
};