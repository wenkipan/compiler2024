#pragma once
#include <ir/Value.hpp>
#include <ir/Function.hpp>
#include <ir/Instrution.hpp>

class BasicBlock : public Value
{
    Function *parent;
    std::vector<PHINode *> *phinodes;

    Instrution *p_branch;
    std::vector<Instrution *> *instrutions;
    // successor in there

public:
    BasicBlock();
    BasicBlock(Function *p_func);
    BasicBlock(std::vector<Instrution *> *instrs);
    ~BasicBlock() override;

    Function *get_func() { return parent; }

    void Set_jmp(BasicBlock *p_next);
    void Set_branch(Value *cond, BasicBlock *p_true, BasicBlock *p_false);

    void Ins_pushFront(Instrution *p_instr);
    void Ins_insert(Instrution *instr, int pos);
    void Ins_pushBack(Instrution *p_instr);
    void Ins_popBack();

    void print() override;
};
