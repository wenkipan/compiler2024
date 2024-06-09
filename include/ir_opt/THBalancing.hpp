#pragma once

#include <functional>
#include <ir/ir.hpp>
#include <unordered_set>

static inline int precedence(Instrution *i)
{
    if (i->get_Instrtype() == InstrutionEnum::IADD)
        return 3;
    else if (i->get_Instrtype() == InstrutionEnum::IMUL)
        return 4;
    assert(0);
}

class THBalancing
{

public:
    std::function<bool(Value *, Value *)> rankcmp = [this](Value *a, Value *b)
    {
        if (get_rank(a) == get_rank(b))
            return a->get_ID() > b->get_ID();
        return get_rank(a) > get_rank(b);
    };
    std::function<bool(Instrution *, Instrution *)> cmp = [](Instrution *a, Instrution *b)
    {
        return precedence(a) < precedence(b);
    };
    THBalancing() : rootqueue(cmp) {}
    void run(Function *func);
    Instrution *balance(Instrution *root);
    int get_rank(Value *i);
    void set_rank(Value *i, int r);
    void Flatten(Instrution *root, Value *r, std::priority_queue<Value *, std::vector<Value *>, decltype(rankcmp)> &q);
    Instrution *rebuild(Instrution *root, std::priority_queue<Value *, std::vector<Value *>, decltype(rankcmp)> &q);

private:
    int if_debug = 1;
    Function *f;
    std::unordered_map<Value *, int> rank;
    std::priority_queue<Instrution *, std::vector<Instrution *>, decltype(cmp)> rootqueue;
    BasicBlock *curbb;
    std::unordered_set<Instrution *> rootset;
    // std::unordered_map<Instrution *, Instrution *> newroots;
};