#pragma once

#include <queue>
#include <unordered_set>

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/DomTree.hpp"

class DCE
{
public:
    int if_debug = 0;
    Function *function;
    PostDomTree *pdt;
    std::unordered_set<Instrution *> live_instr;
    std::unordered_set<BasicBlock *> live_blocks;
    std::unordered_set<BasicBlock *> CFlive_block;
    std::unordered_set<BasicBlock *> phichecked;
    std::queue<Instrution *> worklist;
    std::unordered_set<BasicBlock *> newliveblocks;
    std::unordered_map<BasicBlock *, int> postorder;
    int c;
    ~DCE() { delete pdt; }
    void run(Function *func);
    void init();
    void elimate();
    void print();
    void mark_live_from_CDG();
    void markphilive(PHINode *phi);
    bool is_live(BasicBlock *bb) { return live_blocks.find(bb) != live_blocks.end(); }
    bool is_live(Instrution *bb) { return live_instr.find(bb) != live_instr.end(); }
    void markblocklive(BasicBlock *bb);
    void elimate_block(BasicBlock *bb);
    void caculatepostorder();
    void update_dead_block();
    void search(BasicBlock *n);
};

// delete unreachableblock

class block_DCE
{
public:
    std::unordered_set<BasicBlock *> live_blocks;
    std::queue<BasicBlock *> worklist;
    Function *function;
    block_DCE(Function *func) { function = func; }
    void run()
    {
        worklist.push(function->get_entryBB());
        while (!worklist.empty())
        {
            BasicBlock *cur = worklist.front();
            worklist.pop();

            if (live_blocks.find(cur) != live_blocks.end())
                continue;
            live_blocks.emplace(cur);

            for (auto edge : *cur->get_user_list())
            {
                worklist.push((BasicBlock *)edge->get_user());
            }
        }
        for (auto BB : *function->get_blocks())
        {
            if (live_blocks.find(BB) == live_blocks.end())
            {
                worklist.push(BB);
            }
        }
        while (!worklist.empty())
        {
            BasicBlock *BB = worklist.front();
            worklist.pop();
            BB->drop();
        }
    }
};
