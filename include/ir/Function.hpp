#pragma once
#include <ir/GlobalValue.hpp>
class BasicBlock;
class Param : public Value
{
public:
    Param(Type *type);
};

class Function : public GlobalValue
{
    int currentbblabel;
    std::vector<Param *> *params;

    BasicBlock *entry_block;
    std::vector<BasicBlock *> *blocks;

public:
    Function();
    void params_push_back(Param *p) { params->push_back(p); }
};