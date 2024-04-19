#include <ir/BasicBlock.hpp>

BasicBlock::BasicBlock(Function *p_func)
    : Value(), parent(p_func)
{
    parent->block_pushBack(this);
}

BasicBlock::BasicBlock()
    : Value(){};

void BasicBlock::Set_jmp(BasicBlock *p_next)
{
    p_branch = new Jmp(this, p_next);
    parent->value_pushBack(p_branch);
}

void BasicBlock::Set_branch(Value *cond, BasicBlock *p_true, BasicBlock *p_false)
{
    p_branch = new Branch(cond, p_true, p_false, this);
    parent->value_pushBack(p_branch);
}

void BasicBlock::prevBB_add(BasicBlock *_prev)
{
    prevBBs->emplace_back(_prev);
}

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