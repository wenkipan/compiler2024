#pragma once
#include <ir/Value.hpp>
#include <ir/Instrution.hpp>

class Function;
class BasicBlock : public Value
{
    Function *parent;
    std::vector<BasicBlock *> *prevBBs;
    std::vector<PHINode *> *phinodes;

    std::vector<Instrution *> *instrutions;
    // successor in there

public:
    BasicBlock();
    BasicBlock(Function *p_func);
    BasicBlock(std::vector<Instrution *> *instrs);

    void Ins_pushFront(Instrution *p_instr);
    void Ins_insert(Instrution *instr, int pos);
    void Ins_pushBack(Instrution *p_instr);
};

class BasicBlockEdge
{
};