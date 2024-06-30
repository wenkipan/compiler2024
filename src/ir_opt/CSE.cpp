#include <ir_opt/CSE.hpp>
#include <unordered_map>
#include <queue>

void CSE::FuncDealer(Function *func)
{
    std::unordered_map<Subexp, Value *, SubexpHash> expmap;
    std::set<BasicBlock *> vis;
    std::queue<BasicBlock *> q1, q2;
    q1.push(func->get_entryBB());
    vis.insert(func->get_entryBB());
    while (!q1.empty())
    {
        std::swap(q1, q2);
        while (!q2.empty())
        {
            BasicBlock *curBB = q2.front();
            q2.pop();
            auto instrs = curBB->get_instrs();
            for (Instrution *instr : *instrs)
            {
                if (instr->get_type()->get_type() == TypeEnum::F32)
                    continue;
                Subexp p_exp(instr->get_Instrtype());
                if (instr->isBinary())
                {
                    Binary *tmp = (Binary *)instr;
                    p_exp.src1 = tmp->get_src1(), p_exp.src2 = tmp->get_src2();
                }
                else if (instr->isUnary())
                {
                    Unary *tmp = (Unary *)instr;
                    p_exp.src1 = tmp->get_src();
                }
                else if (instr->isGEP())
                {
                    GEP *tmp = (GEP *)instr;
                    p_exp.src1 = tmp->get_addr(), p_exp.src2 = tmp->get_offset(), p_exp.is_ele = tmp->get_isele();
                }
                else
                    continue;
                auto it = expmap.find(p_exp);
                if (it == expmap.end())
                    expmap.insert({p_exp, instr});
                else
                {
                    instr->print();
                    Value *val = it->second;
                    auto _list = instr->get_user_list();
                    for (Edge *edge : *_list)
                        edge->set_val(val);
                    _list->clear();
                }
            }
            auto _list = curBB->get_user_list();
            for (auto it = _list->begin(); it != _list->end(); it++)
            {
                BasicBlock *nextBB = (BasicBlock *)((*it)->get_user());
                if (vis.find(nextBB) != vis.end())
                    continue;
                vis.insert(nextBB);
                q1.push(nextBB);
            }
        }
    }
}

void CSE::PassRun(Module *_module)
{
    p_module = _module;
    for (Function *p_func : *_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}
