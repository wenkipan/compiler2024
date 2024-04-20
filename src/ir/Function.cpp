#include <ir/Function.hpp>
#include <ir/IRGener.hpp>
#include <iostream>

int Param::CurID = 0;

Param::Param(p_symbol_var p_var)
    : Value(p_var, p_var->p_type->basic),
      loads(new std::vector<Instrution *>)
{
}
/*
Function::Function(p_symbol_func p_func)
    : GlobalValue(p_func), currentbblabel(0)
{
    // params
    entry_block = block_addnewBB();
    blocks->emplace_back(entry_block);

    p_list_head p_node;
    Param::CurID = 0;
    list_for_each(p_node, &p_func->param)
    {
        p_symbol_var p_var = list_entry(p_node, symbol_var, node);
        Param *p_param = new Param(p_var);
        params->emplace_back(p_param);
        values.emplace_back(p_param);
        Value *p_vreg = new Value(p_param->get_type());
        values.emplace_back(p_vreg);
        Instrution *p_instr = new Store(p_param, p_vreg, true, entry_block);
        entry_block->Ins_pushBack(p_instr);
        values.emplace_back(p_instr);
    }
    // ret

    // block
}
*/
template <typename T>
void _IRGen(p_ast_block p_ast_block, Function *func, p_symbol_func _p_func)
{
    T *Gener = new T(func, func->get_entryBB(), _p_func);
    Gener->stmt2ir(p_ast_block);
    delete Gener;
}

Function::Function(p_symbol_func _p_func)
    : GlobalValue(_p_func),
      currentbblabel(0),
      params(new std::vector<Param *>),
      values(new std::vector<Value *>),
      entry_block(nullptr),
      blocks(new std::vector<BasicBlock *>)

{
}

void Function::CallGen(p_ast_block p_ast_block, p_symbol_func _p_func)
{
    entry_block = block_addnewBB();
    Param::CurID = 0;
    _IRGen<GenFunction>(p_ast_block, this, _p_func);
}

void Function::value_pushBack(Value *p_val)
{
    values->emplace_back(p_val);
}

void Function::Param_pushBack(Param *p_param)
{
    params->emplace_back(p_param);
}

BasicBlock *Function::block_addnewBB()
{
    BasicBlock *_BB = new BasicBlock(this);
    blocks->emplace_back(_BB);
    return _BB;
}

void Function::block_pushBack(BasicBlock *p_block)
{
    blocks->emplace_back(p_block);
}

BasicBlock *Function::get_entryBB()
{
    return entry_block;
}

static inline void _params_print(std::vector<Param *> &params)
{
    putchar('(');
    int n = params.size() - 1;
    for (int i = 0; i < n; ++i)
    {
        params[i]->print();
        printf(", ");
    }
    if (n >= 0)
        params[n]->print();
    printf(")\n");
}

void Function::print()
{
    this->get_type()->print();
    std::cout << " @" << this->get_name();
    _params_print((*params));
    printf("{\n");
    for (BasicBlock *p_BB : (*blocks))
        p_BB->print();
    printf("}\n");
}

void Param::print()
{
    this->get_type()->print();
    printf(" %%%d", this->get_ID());
}

Function::~Function()
{
    for (Value *_var : (*values))
        delete _var;
    for (BasicBlock *BB : (*blocks))
        delete BB;

    delete params;
    delete values;
    delete blocks;
}