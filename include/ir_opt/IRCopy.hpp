#pragma once

#include "ir/BasicBlock.hpp"
#include "ir/Function.hpp"
#include <ir/ir.hpp>

class IRCopy
{
    Module *prem;
    Module *newm;
    std::unordered_map<Value *, Value *> instrsmap;
    std::unordered_map<BasicBlock *, BasicBlock *> bbsmap;
    Function *dealingf = nullptr;

public:
    IRCopy() {}
    Function *copy_func(Function *f);
    Value *get_mapval(Value *a);
    BasicBlock *get_mapbb(BasicBlock *a);
    void emplace_val(Value *a, Value *b);
    Module *copy_module();
};