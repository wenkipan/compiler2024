#pragma once
#include <ir/Value.hpp>
#include <ir/Function.hpp>
#include <ir/Instrution.hpp>

void dropInstrs(const std::vector<Instrution *> &dropList);
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
    void drop();
    Function *get_func() { return parent; }
    std::vector<Instrution *> *get_instrs() { return instrutions; }
    std::vector<PHINode *> *get_phis() { return phinodes; }

    void Set_jmp(BasicBlock *p_next);
    void Set_branch(Value *cond, BasicBlock *p_true, BasicBlock *p_false);
    void Set_parent(Function *_func) { parent = _func; }

    void Ins_pushFront(Instrution *p_instr);
    void Ins_insert(Instrution *instr, int pos);
    void Ins_pushBack(Instrution *p_instr);
    void Ins_popBack();
    void Ins_set(int pos, Instrution *p_instr);

    void print() override;

    Instrution *get_last_instrution() { return instrutions->back(); }
    std::vector<PHINode *> *get_phinodes() { return phinodes; }
    std::vector<Instrution *> *get_instrutions() { return instrutions; }

    void Insert_Phi(PHINode *phi);

    void erase_instr(Instrution *instr);
    friend void dropInstrs(const std::vector<Instrution *> &dropList);
};
