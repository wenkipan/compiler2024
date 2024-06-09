#pragma once

#include "ir/BasicBlock.hpp"
#include <ir/ir.hpp>
#include <ir_opt/DomTree.hpp>
#include <ir_opt/Loop.hpp>

class GCM
{
    int debug = 0;
    Function *f;
    std::unordered_set<Instrution *> visited;
    BasicBlock *root;
    DomTree *domtree;
    Loop_Analysis *nesttree;
    std::unordered_map<Instrution *, BasicBlock *> scheduleBB;

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
};