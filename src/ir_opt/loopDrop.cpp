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
    Instrution *p_branch = _BB->get_last_instrution();
    assert(p_branch->get_Instrtype() == InstrutionEnum::Branch);
    BasicBlock *p_next = nullptr;
    if ((*_BB->get_user_list())[0]->get_user() == _BB)
        p_next = (BasicBlock *)(*_BB->get_user_list())[1]->get_user();
    else
        p_next = (BasicBlock *)(*_BB->get_user_list())[0]->get_user();
    std::vector<Edge *> *edges = p_next->get_value_list();
    for (auto it = edges->begin(); it != edges->end(); ++it)
        if ((*it)->get_val() == _BB)
        {
            delete *it;
            edges->erase(it);
            break;
        }
    edges = _BB->get_value_list();
    for (auto it = edges->begin(); edges->end() != it; ++it)
        if ((*it)->get_val() == _BB)
        {
            delete *it;
            edges->erase(it);
            break;
        }
    (*_BB->get_phinodes())[0]->drop();
    for (Instrution *p_instr : (*_BB->get_instrs()))
    {
        edges = p_instr->get_user_list();
        for (Edge *edge : *edges)
            delete edge;
        edges->clear();
    }
    _BB->get_instrs()->clear();
    _BB->get_phinodes()->clear();
    _BB->get_user_list()->clear();
    _BB->Set_jmp(p_next);
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