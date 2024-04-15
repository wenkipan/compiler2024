#pragma once
#include <ir/Value.hpp>
#include <ir/Instrution.hpp>
#include <ir/Function.hpp>
#include <list>

class BasicBlock : public Value
{
    Functon *parent;
    int label;
    std::vector<BasicBlock *> *prevBBs;
    std::vector<PHINode *> *phinodes;
    std::vector<Instrution *> *instrutions;
    // successor in there

public:
    BasicBlock();
    BasicBlock(std::vector<Instrution *> *instrs);
    void push_front_instr();
    void insert_instr(Instrution *instr, int pos);
    void push_back_instr();
};

class BasicBlockEdge
{
};