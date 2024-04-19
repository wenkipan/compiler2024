#include <ir/Function.hpp>
#include <ir/IRGener.hpp>

int Param::CurID = 0;

Param::Param(p_symbol_var p_var)
    : Value(p_var, p_var->p_type->basic)
{
}
/*
Function::Function(p_symbol_func p_func)
    : GlobalValue(p_func), currentbblabel(0)
{
    // params
    entry_block = new BasicBlock(this);
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
}

Function::Function(p_ast_block _p_ast_block, p_symbol_func _p_func)
    : GlobalValue(_p_func), currentbblabel(0)
{
    entry_block = new BasicBlock(this);
    Param::CurID = 0;
    _IRGen<GenFunction>(_p_ast_block, this, _p_func);
}

void Function::value_pushBack(Value *p_val)
{
    values.emplace_back(p_val);
}

void Function::Param_pushBack(Param *p_param)
{
    params->emplace_back(p_param);
}

void Function::block_pushBack(BasicBlock *p_block)
{
    blocks->emplace_back(p_block);
}

BasicBlock *Function::get_entryBB()
{
    return entry_block;
}
