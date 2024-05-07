#include "ir/Value.hpp"
#include <ir/BasicBlock.hpp>
#include <algorithm>
BasicBlock::BasicBlock(Function *p_func)
    : Value(), parent(p_func),
      phinodes(new std::vector<PHINode *>),
      p_branch(nullptr),
      instrutions(new std::vector<Instrution *>)
{
}

BasicBlock::BasicBlock()
    : Value(),
      phinodes(new std::vector<PHINode *>),
      p_branch(nullptr),
      instrutions(new std::vector<Instrution *>){};

void BasicBlock::drop()
{
    // erase from function
    auto blocks = parent->get_blocks();
    blocks->erase(remove(blocks->begin(), blocks->end(), this), blocks->end());
    // erase its instr
    for (auto phi : *phinodes)
        phi->drop();
    for (auto instr : *instrutions)
        instr->drop();
    // erase from cfg(value drop)
    Value::drop();
    delete this;
}

void BasicBlock::Set_jmp(BasicBlock *p_next)
{
    p_branch = new Jmp(p_next, this);
}

void BasicBlock::Set_branch(Value *cond, BasicBlock *p_true, BasicBlock *p_false)
{
    p_branch = new Branch(cond, p_true, p_false, this);
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
    printf("b%d:                       ;", this->get_ID());
    std::vector<Edge *> *_prev = this->get_value_list();
    int n = _prev->size() - 1;
    for (int i = 0; i < n; ++i)
    {
        (*_prev)[i]->get_val()->print_ID();
        printf(", ");
    }

    if (n >= 0)
        (*_prev)[n]->get_val()->print_ID();
    putchar('\n');
    for (PHINode *p_PHI : (*phinodes))
        p_PHI->print();
    for (Instrution *p_instr : (*instrutions))
        p_instr->print();
}

BasicBlock::~BasicBlock()
{

    delete phinodes;
    delete instrutions;
}

void BasicBlock::Insert_Phi(PHINode *phi)
{
    phinodes->push_back(phi);
}

void BasicBlock::erase_instr(Instrution *instr)
{
    for (auto it = instrutions->begin(); it != instrutions->end(); it++)
        if (*it == instr)
        {
            instrutions->erase(it);
            break;
        }
}

void BasicBlock::erase_phi(PHINode *phi)
{
    for (auto it = phinodes->begin(); it != phinodes->end(); it++)
        if (*it == phi)
        {
            phinodes->erase(it);
            break;
        }
}

void BasicBlock::Ins_set(int pos, Instrution *p_instr)
{
    (*instrutions)[pos] = p_instr;
}

void dropInstrs(const std::vector<Instrution *> &dropList)
{
    std::unordered_map<BasicBlock *, std::queue<Instrution *>> dropMap;
    for (auto Instr : dropList)
    {
        if (dropMap.find(Instr->get_BB()) == dropMap.end())
            dropMap.insert(make_pair(Instr->get_BB(), std::queue<Instrution *>()));
        dropMap.find(Instr->get_BB())->second.push(Instr);
    }
    for (auto it : dropMap)
    {
        std::vector<Instrution *> *tmp = new std::vector<Instrution *>();
        std::vector<Instrution *> *removeList = new std::vector<Instrution *>();
        for (auto Instr : *(it.first->get_instrutions()))
            if (!it.second.empty() && Instr == it.second.front())
            {
                it.second.pop();
                removeList->push_back(Instr);
            }
            else
                tmp->push_back(Instr);
        std::swap(it.first->instrutions, tmp);
        delete tmp;
        for (auto it : *removeList)
            dynamic_cast<Value *>(it)->drop();
        delete removeList;
    }
}