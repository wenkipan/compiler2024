#include <ir_opt/SSARegisterAlloc.hpp>

std::vector<int> SSARegisterAlloc::regsStillAliveAfterCall(Call *call)
{
    auto vrs = callLiveVreg[call];
    std::vector<int> ret;
    for (auto it : vrs)
    {
        int c = color[it];
        if (c <= 3 || c == 12 || (c >= 16 && c <= 31))
            ret.push_back(c);
    }
    return ret;
}

int SSARegisterAlloc::getReg(Value *val)
{
    if (LA.ValueIdMap.find(val) == LA.ValueIdMap.end())
        assert(0);
    int x = LA.ValueIdMap.at(val);
    if (color.find(x) == color.end())
        assert(0);
    int c = color[x];
    if (LA.is_float[x])
    {
        return 16 + c;
    }
    else
    {
        if (c <= 10)
            return c;
        else
            return 14;
    }
}

void SSARegisterAlloc::AddBB(Function *p_func)
{
    std::vector<BasicBlock *> allTmpBB;
    for (auto bb : *(p_func->get_blocks()))
    {
        if (dynamic_cast<Branch *>(bb->get_instrutions()->back()) != nullptr)
        {
            assert(bb->get_user_list()->size() == 2);
            auto now_user_list = *(bb->get_user_list());
            BasicBlock *trueBB = dynamic_cast<BasicBlock *>(now_user_list[0]->get_user());
            BasicBlock *falseBB = dynamic_cast<BasicBlock *>(now_user_list[1]->get_user());

            if (!trueBB->get_phinodes()->empty() && !falseBB->get_phinodes()->empty())
            {
                BasicBlock *trueNewBB = new BasicBlock(p_func);
                BasicBlock *falseNewBB = new BasicBlock(p_func);
                allTmpBB.push_back(trueNewBB);
                allTmpBB.push_back(falseNewBB);
                trueNewBB->Set_jmp(trueBB);
                falseNewBB->Set_jmp(falseBB);
                for (auto phi : *(trueBB->get_phinodes()))
                {
                    std::unordered_map<BasicBlock *, Edge *> tmpMap;
                    for (auto it : *(phi->get_valueMap()))
                    {
                        if (it.first == bb)
                            tmpMap.insert(std::make_pair(trueNewBB, it.second));
                        else
                            tmpMap.insert(it);
                    }
                    std::swap(tmpMap, *(phi->get_valueMap()));
                }
                new Edge(trueNewBB, bb);
                new Edge(falseNewBB, bb);
                now_user_list[0]->drop();
                now_user_list[1]->drop();
            }
        }
    }
    for (auto bb : allTmpBB)
        p_func->block_pushBack(bb);
}

Param *SSARegisterAlloc::whichPara(Alloca *alloc)
{
    if (paraMap.find(alloc) == paraMap.end())
        return nullptr;
    else
        return paraMap[alloc];
}

void SSARegisterAlloc::run(Function *p_func)
{
    AddBB(p_func);
    p_func->ResetID(false);
    LA.run(p_func);
    vregNum = LA.Vals.size();
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
    G.resize(vregNum);
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
                // assert(live.find(def) != live.end());
                if (live.find(def) != live.end())
                {
                    live.erase(def);
                    for (auto l : live)
                        AddEdge(l, def);
                }
            }
            if (dynamic_cast<Call *>(*it) != nullptr)
                continue;
            for (auto edge : *((*it)->get_value_list()))
            {
                Value *use = edge->get_val();
                if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                    live.insert(LA.ValueIdMap.at(use));
            }
        }
        if (bb == p_func->get_entryBB())
        {
            for (auto it = p_func->get_params()->rbegin(); it != p_func->get_params()->rend(); it++)
            {
                int def = LA.ValueIdMap.at(*it);
                if (live.find(def) != live.end())
                {
                    live.erase(def);
                    for (auto l : live)
                        AddEdge(l, def);
                }
            }
        }
    }
}

void SSARegisterAlloc::AssignColor_R(Function *p_func)
{
    int n = 0;
    std::vector<int> id_R;
    std::unordered_map<int, int> id_R_map;
    id_R.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_float[i] == 0)
        {
            id_R.push_back(i);
            n++;
            id_R_map[i] = n;
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
        for (auto tmp : G[id_R[x]])
        {
            int y = id_R_map[tmp];
            if (color[id_R[y]] != -1)
                color_set.insert(color[id_R[y]]);
        }
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
        for (auto tmp : G[id_R[x]])
        {
            int y = id_R_map[tmp];
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
}

void SSARegisterAlloc::AssignColor_S(Function *p_func)
{
    int n = 0;
    std::vector<int> id_S;
    std::unordered_map<int, int> id_S_map;
    id_S.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_float[i] == 1)
        {
            id_S.push_back(i);
            n++;
            id_S_map[i] = n;
        }
    std::vector<int> next(n + n + 1), pre(n + n + 1), w(n + 1), q(n + 1), dy(n + 1);
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
        for (auto tmp : G[id_S[x]])
        {
            int y = id_S_map[tmp];
            if (color[id_S[y]] != -1)
                color_set.insert(color[id_S[y]]);
        }
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
        for (auto tmp : G[id_S[x]])
        {
            int y = id_S_map[tmp];
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
}

void SSARegisterAlloc::spillUserPhi(int x)
{
    Value *r = LA.Vals[x];
    for (auto edge : *(r->get_user_list()))
    {
        Value *user = edge->get_user();
        if (dynamic_cast<PHINode *>(user) != nullptr)
        {
            assert(LA.ValueIdMap.find(user) != LA.ValueIdMap.end());
            int y = LA.ValueIdMap[user];
            if (spilledNodes.find(y) == spilledNodes.end())
            {
                spilledNodes.insert(y);
                spillUserPhi(y);
            }
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
            spillUserPhi(it.second);
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
        for (auto op : ops)
        {
            if (inReg.find(op) == inReg.end())
            {
                if (Regs.size() == K_R)
                    for (auto it : Regs)
                    {
                        if (ops.find(it.second) == ops.end())
                        {
                            spilledNodes.insert(it.second);
                            spillUserPhi(it.second);
                            inReg.erase(it.second);
                            Regs.erase(it);
                            break;
                        }
                    }
                if (Regs.size() == K_R)
                {
                    spilledNodes.insert(op);
                    spillUserPhi(op);
                    continue;
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
        for (auto reg : Regs)
        {
            if (spilledNodes.find(reg.second) != spilledNodes.end())
            {
                inReg.erase(reg.second);
                Regs.erase(reg);
            }
            if (LA.OutSet[bb].at(reg.second))
                continue;
            bool live = false;
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
        if (dynamic_cast<Call *>(ins) != nullptr)
        {
            for (auto reg : Regs)
                callLiveVreg[dynamic_cast<Call *>(ins)].push_back(reg.second);
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(ins)] == 0)
        {
            int result = LA.ValueIdMap.at(ins);
            if (Regs.size() == K_R)
            {
                spilledNodes.insert(Regs.begin()->second);
                spillUserPhi(Regs.begin()->second);
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
            spillUserPhi(it.second);
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
        for (auto op : ops)
        {
            if (inReg.find(op) == inReg.end())
            {
                if (Regs.size() == K_S)
                    for (auto it : Regs)
                    {
                        if (ops.find(it.second) == ops.end())
                        {
                            spilledNodes.insert(it.second);
                            spillUserPhi(it.second);
                            inReg.erase(it.second);
                            Regs.erase(it);
                            break;
                        }
                    }
                if (Regs.size() == K_S)
                {
                    spilledNodes.insert(op);
                    spillUserPhi(op);
                    continue;
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
        for (auto reg : Regs)
        {
            if (spilledNodes.find(reg.second) != spilledNodes.end())
            {
                inReg.erase(reg.second);
                Regs.erase(reg);
            }
            if (LA.OutSet[bb].at(reg.second))
                continue;
            bool live = false;
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
        if (dynamic_cast<Call *>(ins) != nullptr)
        {
            for (auto reg : Regs)
                callLiveVreg[dynamic_cast<Call *>(ins)].push_back(reg.second);
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(ins)] == 1)
        {
            int result = LA.ValueIdMap.at(ins);
            if (Regs.size() == K_S)
            {
                spilledNodes.insert(Regs.begin()->second);
                spillUserPhi(Regs.begin()->second);
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
        Alloca *alloc = new Alloca(ebb, LA.Vals[v]->get_type(), 1);
        assert(alloc->get_type()->get_type() == TypeEnum::Ptr);
        alloc->insertInstr(ebb, 0);
        allocMap[v] = alloc;
        if (dynamic_cast<Param *>(LA.Vals[v]) != nullptr)
            paraMap[alloc] = dynamic_cast<Param *>(LA.Vals[v]);
    }

    for (auto v : spilledNodes)
    {
        Value *val = LA.Vals[v];
        PHINode *phi = dynamic_cast<PHINode *>(val);
        Instrution *ins = dynamic_cast<Instrution *>(val);
        // Param *para = dynamic_cast<Param *>(val);
        if (phi != nullptr)
        {
            for (auto it : *(phi->get_valueMap()))
            {
                Store *store = new Store(allocMap[v], it.second->get_val(), false, it.first);
                int insNum = store->get_parent()->get_instrutions()->size();
                if (dynamic_cast<Branch *>(store->get_parent()->get_instrutions()->at(insNum - 2)) != nullptr)
                {
                    store->insertInstr(store->get_parent(), insNum - 3);
                }
                else
                {
                    store->insertInstr(store->get_parent(), insNum - 2);
                }
            }
        }
        else if (ins != nullptr)
        {
            Store *store = new Store(allocMap[v], ins, false, ins->get_parent());
            auto insList = store->get_parent()->get_instrs();
            int pos = std::find(insList->begin(), insList->end(), ins) - insList->begin() + 1;
            store->insertInstr(store->get_parent(), pos);
        }
        /*else if (para != nullptr)
        {
            Store *store = new Store(allocMap[v], para, false, p_func->get_entryBB());
            auto insList = store->get_parent()->get_instrs();
            int pos = std::find(insList->begin(), insList->end(), allocMap[v]) - insList->begin() + 1;
            store->insertInstr(p_func->get_entryBB(), pos);
        }*/
    }
    for (auto v : spilledNodes)
    {
        Value *val = LA.Vals[v];
        std::unordered_map<Instrution *, Load *> load_map;
        for (auto it : *(val->get_user_list()))
        {
            Instrution *use = dynamic_cast<Instrution *>(it->get_user());
            if (dynamic_cast<Store *>(use) != nullptr && dynamic_cast<Store *>(use)->get_addr() == allocMap[v])
            {
                continue;
            }
            auto insList = use->get_parent()->get_instrs();
            if (std::find(insList->begin(), insList->end(), use) != insList->end())
            {
                Load *load;
                if (load_map.find(use) == load_map.end())
                {
                    load = new Load(allocMap[v], false, use->get_parent());
                    int pos = std::find(insList->begin(), insList->end(), use) - insList->begin();
                    load->insertInstr(load->get_parent(), pos);
                    load_map[use] = load;
                }
                else
                    load = load_map[use];
                it->set_val(load);
            }
        }
        Edge *tmp_edge = nullptr;
        for (auto edge : *(val->get_user_list()))
            if (edge->get_val() == val && dynamic_cast<PHINode *>(edge->get_user()) == nullptr)
            {
                tmp_edge = edge;
                break;
            }
        val->get_user_list()->clear();
        if (tmp_edge != nullptr)
            val->get_user_list()->push_back(tmp_edge);
        PHINode *phi = dynamic_cast<PHINode *>(val);
        if (phi != nullptr)
            phi->drop();
    }
}