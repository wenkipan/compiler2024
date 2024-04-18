#pragma once

#include <ir/GlobalValue.hpp>
#include <ir/BasicBlock.hpp>

#include <ast/ast.hpp>

class Param : public Value
{
    int ID = CurID++;
    std::vector<Instrution *> *loads;

public:
    static int CurID;
    Param(p_symbol_var p_var);
};

class Function : public GlobalValue
{
    int currentbblabel;
    std::vector<Param *> *params;

    BasicBlock *entry_block;
    std::vector<Value *> values;
    std::vector<BasicBlock *> *blocks;

    bool isExternal = false;

public:
    Function(p_symbol_func p_func);
    Function(p_ast_block _p_ast_block, p_symbol_func _p_func);

    void Val_pushBack(Value *p_val);

    void BB_addTail(BasicBlock *p_block);

    BasicBlock *get_entryBB();
};
