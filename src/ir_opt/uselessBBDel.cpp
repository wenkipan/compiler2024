#include <ir_opt/uselessBBDel.hpp>
#include <queue>
#include <algorithm>

bool uselessBBDel::run(Function *f)
{
    function = f;
    bool deal = false;
    eliminate_uselessBB();
    return deal;
}

void uselessBBDel::eliminate_uselessBB()
{
    std::queue<BasicBlock *> q;
    std::unordered_map<BasicBlock *, bool> vis;
    q.push(function->get_entryBB());
    vis.insert({function->get_entryBB(), true});
    while (!q.empty())
    {
        BasicBlock *cur = q.front();
        q.pop();
        for (auto it : *cur->get_user_list())
        {
            if (vis.find((BasicBlock *)(it->get_user())) != vis.end())
                continue;
            q.push((BasicBlock *)(it->get_user()));
            vis.insert({(BasicBlock *)(it->get_user()), true});
        }

        if (cur->get_instrs()->size() != 1 || cur->get_user_list()->size() != 1 || cur == function->get_entryBB())
            continue;
        BasicBlock *nextBB = (BasicBlock *)(*cur->get_user_list()->begin())->get_user();
        auto _list = cur->get_value_list();
        bool flag = false;
        for (Edge *edge : *_list)
        {
            BasicBlock *nwBB = (BasicBlock *)edge->get_val();
            if (nwBB->get_last_instrution()->isBranch())
            {
                auto users = nwBB->get_user_list();
                if ((*users)[0]->get_user() == cur && (*users)[1]->get_user() == nextBB)
                    flag = true;
                else if ((*users)[0]->get_user() == nextBB && (*users)[1]->get_user() == cur)
                    flag = true;
            }
        }
        if (nextBB->get_phinodes()->empty())
            flag = false;
        if (flag)
            continue;
        if (cur->get_phinodes()->empty())
        {
            for (PHINode *Phi : *nextBB->get_phinodes())
            {
                Value *val = Phi->get_valueMap()->find(cur)->second->get_val();
                for (auto it = _list->begin(); it != _list->end(); ++it)
                    Phi->addIncoming(val, (BasicBlock *)(*it)->get_val());
            }
        }
        else
        {
            bool onlyNextPhi = true;
            for (PHINode *Phi : *cur->get_phinodes())
            {
                auto users = Phi->get_user_list();
                if (users->size() == 1)
                {
                    Instrution *p_instr = (Instrution *)(*users->begin())->get_user();
                    if (!p_instr->isPHINode() || p_instr->get_parent() != nextBB)
                        onlyNextPhi = false;
                }
                else
                    onlyNextPhi = false;
            }
            if (onlyNextPhi)
            {
                int comenum = 0;
                for (PHINode *Phi : *cur->get_phinodes())
                {
                    auto users = Phi->get_user_list();
                    PHINode *NextBBPhi = (PHINode *)(*users->begin())->get_user();
                    for (auto it = Phi->get_valueMap()->begin(); it != Phi->get_valueMap()->end(); ++it)
                    {
                        assert(NextBBPhi->get_valueMap()->find(it->first) == NextBBPhi->get_valueMap()->end());
                        NextBBPhi->addIncoming(it->second->get_val(), it->first);
                    }
                    Phi->get_user_list()->clear();
                    comenum = NextBBPhi->get_valueMap()->size();
                }
                assert(comenum > 0);
                for (PHINode *Phi : *nextBB->get_phinodes())
                {
                    if (Phi->get_valueMap()->size() == comenum)
                        continue;
                    Constant *const0 = nullptr;
                    if (Phi->get_type()->get_type() == TypeEnum::I32)
                        const0 = new ConstantI32(0);
                    else if (Phi->get_type()->get_type() == TypeEnum::F32)
                        const0 = new ConstantF32(0.0);
                    else
                        assert(0);
                    assert(const0 != nullptr);
                    function->value_pushBack(const0);
                    for (Edge *edge : *_list)
                    {
                        BasicBlock *fromBB = (BasicBlock *)edge->get_val();
                        Phi->addIncoming(const0, fromBB);
                    }
                }
            }
            else
                continue; // to do
        }

        for (Edge *edge : *_list)
        {
            edge->set_user(nextBB);
        }
        cur->get_value_list()->clear();
        function->print();
        puts("\n\n");
    }
}
