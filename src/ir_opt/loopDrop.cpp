#include <ir_opt/loopDrop.hpp>

static bool inline _check(Value *p_val, BasicBlock *BB)
{
    std::vector<Edge *> *edges = p_val->get_user_list();
    for (Edge *edge : *edges)
    {
        Instrution *p_user = (Instrution *)(edge->get_user());
        if (p_user->get_parent() != BB)
            return true;
    }
    return false;
}

void LoopDrop::DealLoop(Loop *loop)
{
    for (auto son : *loop->get_lpsons())
        DealLoop(son);
    if (!loop->get_lpsons()->empty() || loop->get_BBs()->size() != 1)
        return;
    BasicBlock *_BB = loop->get_header();
    if (_BB->get_phinodes()->size() != 1)
        return;
    for (Instrution *p_instr : *_BB->get_instrs())
    {
        if (p_instr->get_Instrtype() == InstrutionEnum::Call || p_instr->get_Instrtype() == InstrutionEnum::Store || p_instr->get_Instrtype() == InstrutionEnum::Load)
            return;
        if (_check(p_instr, _BB))
            return;
    }
    if (_check((*_BB->get_phinodes()->begin()), _BB))
        return;
    assert(_BB->get_last_instrution()->isBranch());
    Branch *p_branch = (Branch *)_BB->get_last_instrution();
    _BB->Ins_popBack();
    bool Texit = true;
    if (p_branch->get_trueBB() == _BB)
        Texit = false;
    Constant *num0 = new ConstantI32(0);
    _BB->get_func()->value_pushBack(num0);
    Instrution *p_cmp = _BB->get_last_instrution();
    assert(p_branch->get_cond() == p_cmp);
    Cmp *n_cmp = nullptr;
    if (Texit)
        n_cmp = new Cmp(InstrutionEnum::IEQ, num0, num0, _BB);
    else
        n_cmp = new Cmp(InstrutionEnum::INEQ, num0, num0, _BB);
    (*p_branch->get_value_list()->begin())->set_val(n_cmp);
    assert(p_cmp->get_user_list()->size() == 1);
    p_cmp->get_user_list()->clear();
    p_cmp->drop();
    _BB->Ins_pushBack(p_branch);
}

void LoopDrop::PassRun(Module *p_module)
{
    loop = new Loop_Analysis();
    loop->PassRun(p_module);
    for (Function *p_func : (*p_module->get_funcs()))
    {
        if (p_func->get_blocks()->empty())
            continue;
        Loop *root = loop->get_LoopInfo()->find(p_func)->second;
        for (Loop *it : (*root->get_lpsons()))
            DealLoop(it);
    }
}

LoopDrop::LoopDrop()
{
}

LoopDrop::~LoopDrop()
{
    delete loop;
}