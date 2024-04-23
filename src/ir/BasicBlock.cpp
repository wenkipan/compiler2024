#include <ir/BasicBlock.hpp>

BasicBlock::BasicBlock(Function *p_func)
    : Value(), parent(p_func),
      prevBBs(new std::vector<BasicBlock *>),
      phinodes(new std::vector<PHINode *>),
      p_branch(nullptr),
      instrutions(new std::vector<Instrution *>)
{
}

BasicBlock::BasicBlock()
    : Value(),
      prevBBs(new std::vector<BasicBlock *>),
      phinodes(new std::vector<PHINode *>),
      p_branch(nullptr),
      instrutions(new std::vector<Instrution *>){};

void BasicBlock::Set_jmp(BasicBlock *p_next)
{
    p_branch = new Jmp(p_next, this);
}

void BasicBlock::Set_branch(Value *cond, BasicBlock *p_true, BasicBlock *p_false)
{
    p_branch = new Branch(cond, p_true, p_false, this);
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

void BasicBlock::Ins_popBack()
{
    instrutions->pop_back();
}

void BasicBlock::print()
{
    printf("%%%d:\n", this->get_ID());
    for (Instrution *p_instr : (*instrutions))
        p_instr->print();
}

BasicBlock::~BasicBlock()
{
    delete prevBBs;
    delete phinodes;
    delete instrutions;
}