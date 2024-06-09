#include <ir/Function.hpp>
#include <ir/IRGener.hpp>
#include <iostream>
#include <unordered_set>

int Param::CurID = 0;

Param::Param(Type *_type)
    : Value(_type, true),
      loads(new std::vector<Instrution *>)
{
}

Param::Param(Type *_type, bool _flag)
    : Value(_type, _flag),
      loads(new std::vector<Instrution *>)
{
}

Param::~Param()
{
    delete loads;
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
        Param *p_param = (p_var);
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
Function::Function(std::string _name)
    : GlobalValue(_name),
      currentbblabel(0),
      params(new std::vector<Param *>),
      values(new std::vector<Value *>),
      entry_block(nullptr),
      blocks(new std::vector<BasicBlock *>)
{
}
Function::Function()
    : GlobalValue(),
      currentbblabel(0),
      params(new std::vector<Param *>),
      values(new std::vector<Value *>),
      entry_block(nullptr),
      blocks(new std::vector<BasicBlock *>)

{
}

void Function::ResetID(bool _flag)
{
    int curlable = 0, curID = 0, voidCnt = 0;
    for (Param *_ptr : (*params))
        _ptr->reset_ID(curID++);
    if (_flag)
        for (BasicBlock *_BB : (*blocks))
        {
            _BB->reset_ID(curlable++);
            auto phis = _BB->get_phis();
            for (PHINode *p_node : (*phis))
            {
                p_node->reset_ID(curID - voidCnt);
                ++curID;
            }
            auto instrs = _BB->get_instrs();
            for (Instrution *p_instr : (*instrs))
            {
                p_instr->reset_ID(curID - voidCnt);
                ++curID;
                if (p_instr->get_type()->get_type() == TypeEnum::Void || p_instr->get_Instrtype() == InstrutionEnum::Ret)
                    ++voidCnt;
            }
        }
    else
        for (BasicBlock *_BB : (*blocks))
        {
            _BB->reset_ID(curlable++);
            auto phis = _BB->get_phis();
            for (PHINode *p_node : (*phis))
            {
                p_node->reset_ID(curID++);
            }
            auto instrs = _BB->get_instrs();
            for (Instrution *p_instr : (*instrs))
                p_instr->reset_ID(curID++);
        }
    Value::CurID = curID;
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

static inline void _params_print(std::vector<Param *> &params)
{
    putchar('(');
    int n = params.size();
    for (int i = 0; i < n - 1; ++i)
    {
        params[i]->get_type()->print();
        printf(" noundef ");
        params[i]->print_ID();
        printf(", ");
    }
    if (n)
    {
        params[n - 1]->get_type()->print();
        printf(" noundef ");
        params[n - 1]->print_ID();
    }
    printf(") #0\n");
}

void Function::print()
{
    std::cout << "define dso_local ";
    this->get_type()->print();
    std::cout << " @" << this->get_name();
    _params_print((*params));
    if (blocks->size())
        printf("{\n");
    for (BasicBlock *p_BB : (*blocks))
        p_BB->print();
    if (blocks->size())
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
    {
        std::vector<Edge *> *_nw = _var->get_user_list();
        for (Edge *_edge : (*_nw))
            delete _edge;
        delete _var;
    }
    for (BasicBlock *BB : (*blocks))
    {
        std::vector<Edge *> *_nw = BB->get_user_list();
        for (Edge *_edge : (*_nw))
            delete _edge;
        delete BB;
    }

    delete params;
    delete values;

    delete blocks;
}
std::vector<Param *> *Function::get_params()
{
    return params;
}

std::vector<Value *> *Function::get_Values()
{
    return values;
}
static inline bool check(BasicBlock *ret, BasicBlock *BB, std::unordered_set<BasicBlock *> *checked)
{
    if (BB == ret)
        return 1;
    checked->emplace(BB);
    int f = 0;
    for (auto edge : *BB->get_user_list())
    {
        BasicBlock *succ = (BasicBlock *)edge->get_user();
        if (checked->find(succ) == checked->end())
            f = f | check(ret, succ, checked);
    }
    return f;
}
bool Function::check_can_ret()
{
    std::unordered_set<BasicBlock *> *checked = new std::unordered_set<BasicBlock *>;
    int f = check(this->get_retBB(), this->get_entryBB(), checked);
    delete checked;
    return f;
}