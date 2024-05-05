#pragma once

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include <ir/ir.hpp>
#include <queue>
#include <unordered_set>
#include <ir_opt/DomTree.hpp>
class DCE
{
public:
    Function *function;
    PostDomTree *pdt;
    std::unordered_set<Instrution *> live_instr;
    std::unordered_set<BasicBlock *> live_blocks;
    std::queue<Instrution *> worklist;
    std::unordered_set<BasicBlock *> newliveblocks;
    DCE(Function *func) { function = func; }
    ~DCE() { delete pdt; }
    void run();
    void init();
    void elimate();
    void print();
    void mark_live_from_CDG();
    bool is_live(BasicBlock *bb) { return live_blocks.find(bb) != live_blocks.end(); }
    bool is_live(Instrution *bb) { return live_instr.find(bb) != live_instr.end(); }
    void elimate_block(BasicBlock *bb);
    void simplifyPHI(PHINode *phi);
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
