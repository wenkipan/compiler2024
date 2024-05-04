#pragma once

#include <ir/ir.hpp>
#include <vector>
#include <unordered_map>
#include <set>

class DomTreeNode
{
    BasicBlock * parent;
    std::vector<DomTreeNode *> *doms;
    std::vector<DomTreeNode *> *idoms;
    DomTreeNode * Sdom;
    DomTreeNode * Idom;
    DomTreeNode * fa;
    DomTreeNode * uni;
    DomTreeNode * mn;
    std::vector<DomTreeNode *> *sdom_graph;
    int dfn;
public:
    DomTreeNode(BasicBlock *BB);
    friend class DomTree;
};

class DomTree
{
    //static std::unordered_map<Function *, DomTree *> *Func_map_Dom;
    std::unordered_map<BasicBlock *, DomTreeNode *> *BB_map_Dom;
    std::vector<DomTreeNode *> *DomTreeNodes;
    Function * parent;
    std::vector<DomTreeNode *> *order;
    int timer;
public:
    DomTree (Function *Func);
    /*static DomTree* get_DomTree(Function *Func)
    {
        return Func_map_Dom->find(Func)->second;
    }*/
    DomTreeNode* get_DomTreeNode(BasicBlock *BB);

    void Run();
    void MakeDom();
    void MakeDomInit();
    void dfs(DomTreeNode *u);
    DomTreeNode* Query_uni(DomTreeNode *u);
    BasicBlock* get_idom(BasicBlock *BB);
    bool is_dom(BasicBlock *A, BasicBlock *B);
    int get_dfn(BasicBlock *BB) { return get_DomTreeNode(BB)->dfn;}
};