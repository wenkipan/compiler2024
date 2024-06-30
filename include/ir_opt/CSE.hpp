#pragma once

#include <ir/ir.hpp>
#include <functional>

struct Subexp
{
    InstrutionEnum type;
    Value *src1, *src2;
    bool is_ele = false;

    Subexp(InstrutionEnum _type)
        : type(_type), src1(nullptr), src2(nullptr)
    {
    }

    Subexp(InstrutionEnum _type, Value *_src1, Value *_src2)
        : type(_type), src1(_src1), src2(_src2)
    {
    }

    bool operator==(const Subexp &b) const
    {
        return type == b.type && src1 == b.src1 && src2 == b.src2 && is_ele == b.is_ele;
    }
};

struct SubexpHash
{
    std::size_t operator()(const Subexp &key) const
    {
        return std::hash<InstrutionEnum>()(key.type) ^ std::hash<Value *>()(key.src1) ^ std::hash<Value *>()(key.src2) ^ std::hash<bool>()(key.is_ele);
    }
};

class CSE
{
    Module *p_module;

public:
    void FuncDealer(Function *func);
    void PassRun(Module *_module);
};