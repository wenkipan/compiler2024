#include <ir/BasicBlock.hpp>

BasicBlock::BasicBlock(Function *p_func)
    : parent(p_func)
{
}

BasicBlock::BasicBlock()
    : Value(){};

void BasicBlock::Ins_insert(Instrution *p_instr, int pos)
{
    instrutions->insert(instrutions->begin() + pos, p_instr);
}

void BasicBlock::Ins_pushFront(Instrution *p_instr)
{
    instrutions->insert(instrutions->begin(), p_instr);
}

void BasicBlock::Ins_pushBack(Instrution *p_instr)
{
    instrutions->emplace_back(p_instr);
}