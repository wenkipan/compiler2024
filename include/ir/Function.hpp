#pragma once
#include <ir/GlobalValue.hpp>
class BasicBlock;
class Param : public Value
{
};

class Functon : public GlobalValue
{
    int currentbblabel;
    std::vector<Param *> *params;

    BasicBlock *entry_block;
    std::vector<BasicBlock *> *blocks;

public:
    Functon();
};