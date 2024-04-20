#pragma once

#include <ir/GlobalValue.hpp>

#include <ast/ast.hpp>

class BasicBlock;
class Instrution;
class Param : public Value
{
    int ID = CurID++;
    std::vector<Instrution *> *loads;

public:
    static int CurID;
    Param(p_symbol_var p_var);

    std::vector<Instrution *> *get_loads() { return loads; }

    void print() override;
};

class Function : public GlobalValue
{
    int currentbblabel;

    std::vector<Param *> *params;
    std::vector<Value *> *values;

    BasicBlock *entry_block;
    std::vector<BasicBlock *> *blocks;

    bool isExternal = false;

public:
    Function(p_symbol_func _p_func);
    ~Function() override;

    void CallGen(p_ast_block p_ast_block, p_symbol_func _p_func);

    void value_pushBack(Value *p_val);
    void Param_pushBack(Param *p_param);

    BasicBlock *block_addnewBB();
    void block_pushBack(BasicBlock *p_block);

    BasicBlock *get_entryBB();
    int get_curBBlabel() { return currentbblabel; }
    bool get_isExternal() { return isExternal; }

    void set_isExternal() { isExternal = true; }

    void print() override;
};
