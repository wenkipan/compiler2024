#pragma once

#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <vector>
#include <unordered_map>
#include <set>

class DomTreeNode
{
    BasicBlock *parent;
    std::vector<DomTreeNode *> *doms;
    std::vector<DomTreeNode *> *idoms;
    DomTreeNode *Sdom;
    DomTreeNode *Idom;
    DomTreeNode *fa;
    DomTreeNode *uni;
    DomTreeNode *mn;
    std::vector<DomTreeNode *> *sdom_graph;
    int dfn;

public:
    DomTreeNode(BasicBlock *BB);
    ~DomTreeNode();
    friend class DomTree;
};

class DomTree
{
    // static std::unordered_map<Function *, DomTree *> *Func_map_Dom;
    std::unordered_map<BasicBlock *, DomTreeNode *> *BB_map_Dom;
    std::vector<DomTreeNode *> *DomTreeNodes;
    Function *parent;
    std::vector<DomTreeNode *> *order;
    int timer;

public:
    DomTree(Function *Func);
    ~DomTree();
    /*static DomTree* get_DomTree(Function *Func)
    {
        return Func_map_Dom->find(Func)->second;
    }*/
    DomTreeNode *get_DomTreeNode(BasicBlock *BB);

    void Run();
    void MakeDom();
    void MakeDomInit();
    void dfs(DomTreeNode *u);
    DomTreeNode *Query_uni(DomTreeNode *u);
    BasicBlock *get_idom(BasicBlock *BB);
    bool is_dom(BasicBlock *A, BasicBlock *B);
    int get_dfn(BasicBlock *BB) { return get_DomTreeNode(BB)->dfn; }
    void get_DF();
    std::unordered_map<BasicBlock *, std::vector<BasicBlock *>> DomFsBlock;
};
class PostDomTree
{
public:
    std::unordered_map<BasicBlock *, BasicBlock *> blockmap;
    Function *parent = new Function;
    PostDomTree(Function *f);
    DomTree *pdt;
    BasicBlock *exit;
    BasicBlock *entry;
    ~PostDomTree();
};
