#include <ir_opt/RegisterAlloc.hpp>

void RegisterAlloc::AddEdge(int u, int v)
{
    if (u != v && adjSet.find(std::make_pair(u, v)) == adjSet.end())
    {
        adjSet.insert(std::make_pair(u, v));
        adjSet.insert(std::make_pair(v, u));
        if (precolored.find(u) == precolored.end())
        {
            adjList[u].insert(v);
            if (degree.find(u) == degree.end())
                degree.insert(std::make_pair(u, 1));
            else
                degree[u]++;
        }
        if (precolored.find(v) == precolored.end())
        {
            adjList[v].insert(u);
            if (degree.find(v) == degree.end())
                degree.insert(std::make_pair(v, 1));
            else
                degree[v]++;
        }
    }
}

void RegisterAlloc::build(Function *p_func)
{
    for (int i = 0; i < 4; i++)
    {
        precolored.insert(i);
        color[i] = i;
    }
    for (int i = 4; i < vregNum; i++)
        initial.insert(i);
    int param_num = 0;
    for (auto para : *(p_func->get_params()))
    {
        if (param_num < 4)
        {
            int tmp = LA.ValueIdMap[para];
            moveList[param_num].insert(std::make_pair(tmp, param_num));
            moveList[tmp].insert(std::make_pair(tmp, param_num));
            worklistMoves.insert(std::make_pair(tmp, param_num));
        }
        param_num++;
    }
    for (BasicBlock *bb : *(p_func->get_blocks()))
    {
        std::unordered_set<int> live;
        for (int i = 0; i < LA.OutSet[bb].size(); i++)
            if (LA.OutSet[bb].at(i))
                live.insert(i);
        for (auto it = bb->get_instrutions()->rbegin(); it != bb->get_instrutions()->rend(); it++)
        {
            auto I = *it;
            if (LA.ValueIdMap.find(I) != LA.ValueIdMap.end())
            {
                if (dynamic_cast<Call *>(I))
                {
                    for (auto edge : *(I->get_user_list()))
                    {
                        Value *use = edge->get_user();
                        if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                            live.erase(LA.ValueIdMap.at(use));
                    }
                    for (auto edge : *(I->get_user_list()))
                    {
                        Value *use = edge->get_user();
                        if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                            moveList[LA.ValueIdMap.at(use)].insert(std::make_pair(LA.ValueIdMap.at(I), 0));
                    }
                    moveList[LA.ValueIdMap.at(I)].insert(std::make_pair(LA.ValueIdMap.at(I), 0));
                    worklistMoves.insert(std::make_pair(LA.ValueIdMap.at(I), 0));
                }
                int def = LA.ValueIdMap.at(I);
                live.insert(def);
                for (auto l : live)
                    AddEdge(l, def);
                live.erase(def);
            }
            for (auto edge : *(I->get_value_list()))
            {
                Value *use = edge->get_user();
                if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                    live.insert(LA.ValueIdMap.at(use));
            }
        }
        std::unordered_set<BasicBlock *> pre_bbs;
        for (auto phi : *(bb->get_phis()))
        {
            for (auto it : *(phi->get_valueMap()))
            {
                pre_bbs.insert(it.first);
            }
        }
        for (auto pre_bb : pre_bbs)
        {
            auto tmp = live;
            for (auto phi : *(bb->get_phis()))
            {
                for (auto it : *(phi->get_valueMap()))
                {
                    if (it.first != pre_bb)
                    {
                        live.erase(LA.ValueIdMap[it.second->get_user()]);
                        live.insert(LA.ValueIdMap[it.second->get_val()]);
                    }
                }
            }
            for (auto phi : *(bb->get_phis()))
            {
                for (auto it : *(phi->get_valueMap()))
                {
                    if (it.first == pre_bb)
                    {
                        int def = LA.ValueIdMap[it.second->get_user()];
                        int use = LA.ValueIdMap[it.second->get_val()];
                        if (!use)
                            continue;
                        live.erase(use);
                        moveList[def].insert(std::make_pair(def, use));
                        moveList[use].insert(std::make_pair(def, use));
                        worklistMoves.insert(std::make_pair(def, use));
                        live.insert(def);
                        for (auto l : live)
                            AddEdge(l, def);
                        live.erase(def);
                        live.insert(use);
                    }
                }
            }
            std::swap(tmp, live);
        }
    }
    std::unordered_set<int> live;
    for (int i = 0; i < LA.InSet[p_func->get_entryBB()].size(); i++)
        if (LA.OutSet[p_func->get_entryBB()].at(i))
            live.insert(i);
    for (auto u : live)
        for (auto v : live)
            AddEdge(u, v);
}

std::unordered_set<std::pair<int, int>, pair_hash> RegisterAlloc::NodeMoves(int n)
{
    std::unordered_set<std::pair<int, int>, pair_hash> tmp = activeMoves;
    std::unordered_set<std::pair<int, int>, pair_hash> ret;
    for (auto it : worklistMoves)
        tmp.insert(it);
    for (auto it : moveList[n])
        if (tmp.find(it) != tmp.end())
            ret.insert(it);
    return ret;
}
bool RegisterAlloc::MoveRelated(int n)
{
    return !NodeMoves(n).empty();
}
void RegisterAlloc::MakeWorklist()
{
    for (auto n : initial)
    {
        if (degree[n] >= K)
            spillWorklist.insert(n);
        else if (MoveRelated(n))
            freezeWorklist.insert(n);
        else
            simplifyWorklist.insert(n);
    }
}

std::unordered_set<int> RegisterAlloc::Adjacent(int n)
{
    std::unordered_set<int> tmp = coalescedNodes, ret;
    for (auto it : selectStack)
        tmp.insert(it);
    for (auto it : adjList[n])
        if (tmp.find(it) == tmp.end())
            ret.insert(it);
    return ret;
}

void RegisterAlloc::Simplify()
{
    auto n = *simplifyWorklist.begin();
    simplifyWorklist.erase(n);
    selectStack.push_back(n);
    for (auto m : Adjacent(n))
        DecrementDegree(m);
}

void RegisterAlloc::DecrementDegree(int m)
{
    int d = degree[m];
    degree[m] = d - 1;
    if (d == K)
    {
        auto tmp = Adjacent(m);
        tmp.insert(m);
        EnableMoves(tmp);
        spillWorklist.erase(m);
        if (MoveRelated(m))
            freezeWorklist.insert(m);
        else
            simplifyWorklist.insert(m);
    }
}

void RegisterAlloc::EnableMoves(std::unordered_set<int> nodes)
{
    for (auto n : nodes)
    {
        for (auto m : NodeMoves(n))
            if (activeMoves.find(m) != activeMoves.end())
            {
                activeMoves.erase(m);
                worklistMoves.insert(m);
            }
    }
}

bool RegisterAlloc::any_t_in_adjv_ok_tu(int u, int v)
{
    for (auto t : Adjacent(v))
    {
        if (!OK(t, u))
            return false;
    }
    return true;
}

std::unordered_set<int> set_union(std::unordered_set<int> Q1, std::unordered_set<int> Q2)
{
    std::unordered_set ret = Q1;
    for (auto it : Q2)
        ret.insert(it);
    return ret;
}
void RegisterAlloc::Coalesce()
{
    auto m = *worklistMoves.begin();
    int x = m.first;
    int y = m.second;
    x = GetAlias(x);
    y = GetAlias(y);
    int u, v;
    if (precolored.find(y) != precolored.end())
    {
        u = y;
        v = x;
    }
    else
    {
        u = x;
        v = y;
    }
    worklistMoves.erase(m);
    if (u == v)
    {
        coalescedMoves.insert(m);
        AddWorkList(u);
    }
    else if (precolored.find(v) != precolored.end() || adjSet.find(std::make_pair(u, v)) != adjSet.end())
    {
        constrainedMoves.insert(m);
        AddWorkList(u);
        AddWorkList(v);
    }
    else if ((precolored.find(u) != precolored.end() && any_t_in_adjv_ok_tu(u, v)) || (precolored.find(u) == precolored.end() && Conservative(set_union(Adjacent(u), Adjacent(v)))))
    {
        coalescedMoves.insert(m);
        Combine(u, v);
        AddWorkList(u);
    }
    else
        activeMoves.insert(m);
}

void RegisterAlloc::AddWorkList(int u)
{
    if (precolored.find(u) == precolored.end() && !MoveRelated(u) && degree[u] < K)
    {
        freezeWorklist.erase(u);
        simplifyWorklist.insert(u);
    }
}

bool RegisterAlloc::OK(int t, int r)
{
    return degree[t] < K || precolored.find(t) != precolored.end() || adjSet.find(std::make_pair(t, r)) != adjSet.end();
}

bool RegisterAlloc::Conservative(std::unordered_set<int> nodes)
{
    int k = 0;
    for (auto n : nodes)
        if (degree[n] >= K)
            k++;
    return k < K;
}

int RegisterAlloc::GetAlias(int n)
{
    if (coalescedNodes.find(n) != coalescedNodes.end())
        return GetAlias(alias[n]);
    else
        return n;
}

void RegisterAlloc::Combine(int u, int v)
{
    if (freezeWorklist.find(v) != freezeWorklist.end())
    {
        freezeWorklist.erase(v);
    }
    else
    {
        spillWorklist.erase(v);
    }
    coalescedNodes.insert(v);
    alias[v] = u;
    for (auto it : moveList[v])
        moveList[u].insert(it);
    std::unordered_set<int> tmp;
    tmp.insert(v);
    EnableMoves(tmp);
    for (auto t : Adjacent(v))
    {
        AddEdge(t, u);
        DecrementDegree(t);
    }
    if (degree[u] >= K && freezeWorklist.find(u) != freezeWorklist.end())
    {
        freezeWorklist.erase(u);
        spillWorklist.insert(u);
    }
}

void RegisterAlloc::Freeze()
{
    auto u = *freezeWorklist.begin();
    freezeWorklist.erase(u);
    simplifyWorklist.insert(u);
    FreezeMoves(u);
}

void RegisterAlloc::FreezeMoves(int u)
{
    for (auto m : NodeMoves(u))
    {
        int x = m.first;
        int y = m.second;
        int v;
        if (GetAlias(y) == GetAlias(u))
        {
            v = GetAlias(x);
        }
        else
        {
            v = GetAlias(y);
        }
        activeMoves.erase(m);
        frozenMoves.insert(m);
        if (NodeMoves(v).empty() && degree[v] < K)
        {
            freezeWorklist.erase(v);
            simplifyWorklist.insert(v);
        }
    }
}

void RegisterAlloc::SelectSpill()
{
    // todo a better select way
    auto m = *spillWorklist.begin();
    spillWorklist.erase(m);
    simplifyWorklist.insert(m);
    FreezeMoves(m);
}

void RegisterAlloc::AssignColors()
{
    while (!selectStack.empty())
    {
        int n = selectStack.back();
        selectStack.pop_back();
        std::unordered_set<int> okColors;
        for (int i = 4; i < K; i++)
            okColors.insert(i);
        for (auto w : adjList[n])
        {
            if (coloredNodes.find(GetAlias(w)) != coloredNodes.end() || precolored.find(GetAlias(w)) != precolored.end())
            {
                okColors.erase(color[GetAlias(w)]);
            }
        }
        if (okColors.empty())
        {
            spilledNodes.insert(n);
        }
        else
        {
            coloredNodes.insert(n);
            int c = *okColors.begin();
            color[n] = c;
        }
    }
    for (auto n : coalescedNodes)
    {
        color[n] = color[GetAlias(n)];
    }
}

void RegisterAlloc::RewriteProgram(Function *p_func)
{
    BasicBlock *ebb = p_func->get_entryBB();
    std::unordered_map<int, Alloca *> allocMap;
    for (auto v : spilledNodes)
    {
        Alloca *alloc = new Alloca(ebb, LA.Vals[v]->get_type(), 0);
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

void RegisterAlloc::run(Function *p_func)
{
    LA.run(p_func);
    vregNum = LA.Vals.size();
    build(p_func);
    return;
    MakeWorklist();
    do
    {
        if (!simplifyWorklist.empty())
            Simplify();
        else if (!worklistMoves.empty())
            Coalesce();
        else if (!freezeWorklist.empty())
            Freeze();
        else if (!spillWorklist.empty())
            SelectSpill();
    } while (!simplifyWorklist.empty() || !worklistMoves.empty() || !freezeWorklist.empty() || !spillWorklist.empty());
    AssignColors();
    if (!spillWorklist.empty())
    {
        RewriteProgram(p_func);
        run(p_func);
    }
}