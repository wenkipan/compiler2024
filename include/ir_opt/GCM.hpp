#pragma once

#include "../../include/ir/ir.hpp"
#include "../../include/ir_opt/DomTree.hpp"
#include "../../include/ir_opt/Loop.hpp"

class GCM
{
    int debug = 0;
    Function *f;
    std::unordered_set<Instrution *> visited;
    BasicBlock *root;
    DomTree *domtree;
    Loop_Analysis *nesttree;
    std::unordered_map<Instrution *, BasicBlock *> scheduleBB;
    std::unordered_map<Function *, bool> sideeffect_func;
    std::unordered_map<BasicBlock *, bool> if_call_bb;

    std::unordered_map<BasicBlock *, bool> hasCall;

public:
    ~GCM();
    void init_visit_with_pinned();
    void init(Function *func);
    void run(Function *func);
    bool ispinned(Instrution *instr);
    void set_scheduleBB(Instrution *i, BasicBlock *bb);
    BasicBlock *get_scheduleBB(Instrution *i);
    void schedule_early(Instrution *instr);
    void schedule_late(Instrution *instr);
    BasicBlock *find_LCA(BasicBlock *, BasicBlock *);
    void schedule_to_block(Instrution *instr, BasicBlock *b);
    void move_instr_to_best();
    void maintain_branch_cond();
    void buildHasCall(BasicBlock *BB);
};