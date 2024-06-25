#include <ir_opt/SSARegisterAlloc.hpp>

void SSARegisterAlloc::run(Function *p_func)
{
    p_func->ResetID(false);
    LA.run(p_func);
    vregNum = LA.Vals.size();
    if (vregNum > 1000)
        return;
    K_R = 13;
    K_S = 32;
    Spill(p_func);
    RewriteProgram(p_func);
    MakeGraph(p_func);
    AssignColor_R(p_func);
    AssignColor_S(p_func);
}

void SSARegisterAlloc::AddEdge(int x, int y)
{
    if (x == y || LA.is_float[x] != LA.is_float[y] || AdjSet.find(std::make_pair(x, y)) != AdjSet.end())
        return;
    AdjSet.insert(std::make_pair(x, y));
    AdjSet.insert(std::make_pair(y, x));
    G[x].push_back(y);
    G[y].push_back(x);
}

void SSARegisterAlloc::MakeGraph(Function *p_func)
{
    p_func->ResetID(false);
    LA.run(p_func);
    vregNum = LA.Vals.size();
    G.resize(vregNum + 1);
    for (BasicBlock *bb : *(p_func->get_blocks()))
    {
        std::unordered_set<int> live;
        for (int i = 0; i < vregNum; i++)
            if (LA.OutSet[bb].at(i))
                live.insert(i);
        for (auto it = bb->get_instrs()->rbegin(); it != bb->get_instrs()->rend(); it++)
        {
            if (LA.ValueIdMap.find(*it) != LA.ValueIdMap.end())
            {
                int def = LA.ValueIdMap.at(*it);
                for (auto l : live)
                    AddEdge(l, def);
                live.erase(def);
            }
            for (auto edge : *((*it)->get_value_list()))
            {
                Value *use = edge->get_user();
                if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                    live.insert(LA.ValueIdMap.at(use));
            }
        }
        if (bb == p_func->get_entryBB())
        {
            for (auto it = p_func->get_params()->rbegin(); it != p_func->get_params()->rend(); it++)
            {
                int def = LA.ValueIdMap.at(*it);
                for (auto l : live)
                    AddEdge(l, def);
                live.erase(def);
            }
        }
    }
}

void SSARegisterAlloc::AssignColor_R(Function *p_func)
{
    int n = 0;
    std::vector<int> id_R;
    id_R.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_float[i] == 0)
        {
            id_R.push_back(i);
            n++;
        }
    std::vector<int> next(n + n + 1), pre(n + n + 1), w(n + 1), q(n + 1), dy(n + 1);
    for (int i = 1; i <= n; i++)
    {
        color[id_R[i]] = -1;
        pre[next[i] = next[n + w[i]]] = i;
        next[pre[i] = n + w[i]] = i;
    }
    int now = 0;
    for (int k = n; k; --k, ++now)
    {
        while (!next[n + now])
            --now;
        int x = next[n + now];
        std::unordered_set<int> color_set;
        for (auto y : G[id_R[x]])
            if (color[id_R[y]] != -1)
                color_set.insert(color[id_R[y]]);
        for (int i = 0; i < K_R; i++)
            if (color_set.find(i) == color_set.end())
            {
                color[id_R[x]] = i;
                break;
            }
        if (color[id_R[x]] == -1)
            assert(0);
        pre[next[x]] = pre[x];
        next[pre[x]] = next[x];
        q[k] = x;
        dy[x] = k;
        for (auto y : G[id_R[x]])
            if (!dy[y])
            {
                pre[next[y]] = pre[y];
                next[pre[y]] = next[y];
                ++w[y];
                pre[next[y] = next[n + w[y]]] = y;
                next[pre[y] = n + w[y]] = y;
            }
    }
}

void SSARegisterAlloc::AssignColor_S(Function *p_func)
{
    int n = 0;
    std::vector<int> id_S;
    id_S.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_float[i] == 1)
        {
            id_S.push_back(i);
            n++;
        }
    std::vector<int> next(n << 1), pre(n << 1), w(n), q(n), dy(n);
    for (int i = 1; i <= n; i++)
    {
        color[id_S[i]] = -1;
        pre[next[i] = next[n + w[i]]] = i;
        next[pre[i] = n + w[i]] = i;
    }
    int now = 0;
    for (int k = n; k; --k, ++now)
    {
        while (!next[n + now])
            --now;
        int x = next[n + now];
        std::unordered_set<int> color_set;
        for (auto y : G[id_S[x]])
            if (color[id_S[y]] != -1)
                color_set.insert(color[id_S[y]]);
        for (int i = 0; i < K_S; i++)
            if (color_set.find(i) == color_set.end())
            {
                color[id_S[x]] = i;
                break;
            }
        if (color[id_S[x]] == -1)
            assert(0);
        pre[next[x]] = pre[x];
        next[pre[x]] = next[x];
        q[k] = x;
        dy[x] = k;
        for (auto y : G[id_S[x]])
            if (!dy[y])
            {
                pre[next[y]] = pre[y];
                next[pre[y]] = next[y];
                ++w[y];
                pre[next[y] = next[n + w[y]]] = y;
                next[pre[y] = n + w[y]] = y;
            }
    }
}

void SSARegisterAlloc::SpillBB_R(BasicBlock *bb)
{
    std::set<std::pair<int, int>> I_B;
    for (int i = 0; i < vregNum; i++)
        if (LA.InSet[bb].at(i) && LA.is_float[i] == 0)
            I_B.insert(std::make_pair(LA.InDis[bb][i], i));
    std::set<std::pair<int, int>> Regs;
    std::unordered_set<int> inReg;
    for (auto it : I_B)
    {
        if (Regs.size() < K_R)
        {
            Regs.insert(std::make_pair(-it.first, it.second));
            inReg.insert(it.second);
        }
        else
        {
            spilledNodes.insert(it.second);
        }
    }
    for (auto ins : *(bb->get_instrs()))
    {
        std::unordered_set<int> ops;
        for (auto edge : *(ins->get_value_list()))
        {
            if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 0)
                ops.insert(LA.ValueIdMap.at(edge->get_val()));
        }
        if (ops.size() <= K_R)
        {
            for (auto op : ops)
            {
                if (inReg.find(op) == inReg.end())
                {
                    for (auto it : Regs)
                    {
                        if (ops.find(it.second) == ops.end())
                        {
                            spilledNodes.insert(it.second);
                            inReg.erase(it.second);
                            Regs.erase(it);
                            break;
                        }
                    }
                    int tmp = LA.OutDis[bb][op] + bb->get_instrs()->size();
                    for (auto edge : *(LA.Vals[op]->get_user_list()))
                    {
                        Value *use = edge->get_user();
                        if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                        {
                            tmp = std::min(tmp, use->get_ID() - bb->get_instrs()->front()->get_ID());
                            break;
                        }
                    }
                    Regs.insert(std::make_pair(-tmp, op));
                    inReg.insert(op);
                }
            }
        }
        else
        {
            // todo
        }
        for (auto reg : Regs)
        {
            if (LA.OutSet[bb].at(reg.second))
                continue;
            bool live = false;
            if (spilledNodes.find(reg.second) != spilledNodes.end())
                live = true;
            else
                for (auto edge : *(LA.Vals[reg.second]->get_user_list()))
                {
                    Value *use = edge->get_user();
                    if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                    {
                        live = true;
                        break;
                    }
                }
            if (!live)
            {
                inReg.erase(reg.second);
                Regs.erase(reg);
            }
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(ins)] == 0)
        {
            int result = LA.ValueIdMap.at(ins);
            if (Regs.size() == K_R)
            {
                inReg.erase(Regs.begin()->second);
                Regs.erase(Regs.begin());
            }
            int tmp = LA.OutDis[bb][result] + bb->get_instrs()->size();
            for (auto edge : *(LA.Vals[result]->get_user_list()))
            {
                Value *use = edge->get_user();
                if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                {
                    tmp = std::min(tmp, use->get_ID() - bb->get_instrs()->front()->get_ID());
                    break;
                }
            }
            Regs.insert(std::make_pair(-tmp, result));
            inReg.insert(result);
        }
    }
}

void SSARegisterAlloc::SpillBB_S(BasicBlock *bb)
{
    std::set<std::pair<int, int>> I_B;
    for (int i = 0; i < vregNum; i++)
        if (LA.InSet[bb].at(i) && LA.is_float[i] == 1)
            I_B.insert(std::make_pair(LA.InDis[bb][i], i));
    std::set<std::pair<int, int>> Regs;
    std::unordered_set<int> inReg;
    for (auto it : I_B)
    {
        if (Regs.size() < K_S)
        {
            Regs.insert(std::make_pair(-it.first, it.second));
            inReg.insert(it.second);
        }
        else
        {
            spilledNodes.insert(it.second);
        }
    }
    for (auto ins : *(bb->get_instrs()))
    {
        std::unordered_set<int> ops;
        for (auto edge : *(ins->get_value_list()))
        {
            if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 1)
                ops.insert(LA.ValueIdMap.at(edge->get_val()));
        }
        if (ops.size() <= K_S)
        {
            for (auto op : ops)
            {
                if (inReg.find(op) == inReg.end())
                {
                    for (auto it : Regs)
                    {
                        if (ops.find(it.second) == ops.end())
                        {
                            spilledNodes.insert(it.second);
                            inReg.erase(it.second);
                            Regs.erase(it);
                            break;
                        }
                    }
                    int tmp = LA.OutDis[bb][op] + bb->get_instrs()->size();
                    for (auto edge : *(LA.Vals[op]->get_user_list()))
                    {
                        Value *use = edge->get_user();
                        if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                        {
                            tmp = std::min(tmp, use->get_ID() - bb->get_instrs()->front()->get_ID());
                            break;
                        }
                    }
                    Regs.insert(std::make_pair(-tmp, op));
                    inReg.insert(op);
                }
            }
        }
        else
        {
            // todo
        }
        for (auto reg : Regs)
        {
            if (LA.OutSet[bb].at(reg.second))
                continue;
            bool live = false;
            if (spilledNodes.find(reg.second) != spilledNodes.end())
                live = true;
            else
                for (auto edge : *(LA.Vals[reg.second]->get_user_list()))
                {
                    Value *use = edge->get_user();
                    if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                    {
                        live = true;
                        break;
                    }
                }
            if (!live)
            {
                inReg.erase(reg.second);
                Regs.erase(reg);
            }
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(ins)] == 1)
        {
            int result = LA.ValueIdMap.at(ins);
            if (Regs.size() == K_S)
            {
                inReg.erase(Regs.begin()->second);
                Regs.erase(Regs.begin());
            }
            int tmp = LA.OutDis[bb][result] + bb->get_instrs()->size();
            for (auto edge : *(LA.Vals[result]->get_user_list()))
            {
                Value *use = edge->get_user();
                if (use->get_ID() > ins->get_ID() && use->get_ID() <= bb->get_instrs()->back()->get_ID())
                {
                    tmp = std::min(tmp, use->get_ID() - bb->get_instrs()->front()->get_ID());
                    break;
                }
            }
            Regs.insert(std::make_pair(-tmp, result));
            inReg.insert(result);
        }
    }
}

void SSARegisterAlloc::Spill(Function *p_func)
{
    for (auto bb : *(p_func->get_blocks()))
    {
        SpillBB_R(bb);
        SpillBB_S(bb);
    }
}

void SSARegisterAlloc::RewriteProgram(Function *p_func)
{
    BasicBlock *ebb = p_func->get_entryBB();
    std::unordered_map<int, Alloca *> allocMap;
    for (auto v : spilledNodes)
    {
        LA.Vals[v]->get_type()->print();
        Alloca *alloc = new Alloca(ebb, LA.Vals[v]->get_type(), 1);
        assert(alloc->get_type()->get_type() == TypeEnum::Ptr);
        alloc->insertInstr(ebb, 0);
        allocMap[v] = alloc;
    }
    for (auto v : spilledNodes)
    {
        Value *val = LA.Vals[v];
        for (auto it : *(val->get_user_list()))
        {
            Instrution *use = dynamic_cast<Instrution *>(it->get_user());
            Load *load = new Load(allocMap[v], false, use->get_parent());
            auto insList = load->get_parent()->get_instrs();
            int pos = std::find(insList->begin(), insList->end(), use) - insList->begin();
            load->insertInstr(load->get_parent(), pos);
            it->set_val(load);
        }
        val->get_user_list()->clear();
    }
    for (auto v : spilledNodes)
    {
        Value *val = LA.Vals[v];
        PHINode *phi = dynamic_cast<PHINode *>(val);
        Instrution *ins = dynamic_cast<Instrution *>(val);
        Param *para = dynamic_cast<Param *>(val);
        if (phi)
        {
            for (auto it : *(phi->get_valueMap()))
            {
                Store(allocMap[v], it.second->get_val(), false, it.first);
            }
            phi->drop();
        }
        else if (ins)
        {
            Store *store = new Store(allocMap[v], ins, false, ins->get_parent());
            auto insList = store->get_parent()->get_instrs();
            int pos = std::find(insList->begin(), insList->end(), ins) - insList->begin() + 1;
            store->insertInstr(store->get_parent(), pos);
        }
        else if (para)
        {
            Store *store = new Store(allocMap[v], para, false, p_func->get_entryBB());
            auto insList = store->get_parent()->get_instrs();
            int pos = std::find(insList->begin(), insList->end(), allocMap[v]) - insList->begin() + 1;
            store->insertInstr(p_func->get_entryBB(), pos);
        }
    }
}