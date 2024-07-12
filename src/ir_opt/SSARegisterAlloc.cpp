#include <ir_opt/SSARegisterAlloc.hpp>

std::vector<int> SSARegisterAlloc::regsStillAliveAfterCall(Call *call)
{
    auto vrs = callLiveVreg[call];
    std::vector<int> ret;
    for (auto it : vrs)
    {
        if (spilledNodes.find(it) != spilledNodes.end())
            continue;
        int c = color[it];
        if (c <= 3 || c == 12 || (c >= 16 && c <= 31))
            ret.push_back(c);
    }
    return ret;
}

int SSARegisterAlloc::getReg(Value *val)
{
    if (std::find(Register.begin(), Register.end(), val) != Register.end())
        return std::find(Register.begin(), Register.end(), val) - Register.begin();
    if (LA.ValueIdMap.find(val) == LA.ValueIdMap.end())
    {
        if (valueMapRegister.find(val) == valueMapRegister.end())
            return -1;
        else
            return valueMapRegister[val];
    }
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

            if (!trueBB->get_phinodes()->empty() || !falseBB->get_phinodes()->empty())
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
                for (auto phi : *(falseBB->get_phinodes()))
                {
                    std::unordered_map<BasicBlock *, Edge *> tmpMap;
                    for (auto it : *(phi->get_valueMap()))
                    {
                        if (it.first == bb)
                            tmpMap.insert(std::make_pair(falseNewBB, it.second));
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
    Register.resize(48);
    for (int i = 0; i < 48; i++)
    {
        Register[i] = new Value(i < 16 ? TypeEnum::I32 : TypeEnum::F32);
        p_func->value_pushBack(Register[i]);
    }

    ReSortForPara(p_func);
    std::vector<Call *> calls;
    for (auto bb : *(p_func->get_blocks()))
    {
        for (auto ins : *(bb->get_instrs()))
            if (dynamic_cast<Call *>(ins) != nullptr)
                calls.push_back(dynamic_cast<Call *>(ins));
    }
    for (auto call : calls)
        ReSortForCall(call);

    for (auto bb : *(p_func->get_blocks()))
        ReSortForPhi(bb);

    /*for (auto it : LA.Vals)
    {
        printf("R %d : ", getReg(it));
        it->print();
    }
    for (auto it : valueMapRegister)
    {
        printf("R %d ", it.second);
        it.first->print();
    }
    for (int i = 0; i < 48; i++)
    {
        printf("R %d ", i);
        Register[i]->print();
    }*/
}

void SSARegisterAlloc::ReSortForPara(Function *p_func)
{
    int nowPos = 0;
    for (auto it : *(p_func->get_entryBB()->get_instrs()))
        if (dynamic_cast<Alloca *>(it) != nullptr)
            nowPos++;
        else
            break;
    auto paras = p_func->get_params();
    std::vector<int> d(Para_num, 0);
    std::vector<Value *> In(Para_num, nullptr);
    for (int i = 0; i < paras->size(); i++)
    {
        if (i < Para_num)
        {
            if (spilledNodes.find(LA.ValueIdMap[paras->at(i)]) != spilledNodes.end())
            {
                Store *store = new Store(allocMap[LA.ValueIdMap[paras->at(i)]], Register[i], false, p_func->get_entryBB());
                store->insertInstr(p_func->get_entryBB(), nowPos++);
            }
            else
            {
                int tmpR = getReg(paras->at(i));
                if (tmpR >= Para_num)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[tmpR], Register[i], p_func->get_entryBB());
                    move->insertInstr(p_func->get_entryBB(), nowPos++);
                }
                else
                {
                    if (i != tmpR)
                    {
                        d[i]++;
                        In[tmpR] = Register[i];
                        printf("yes %d %d\n", tmpR, i);
                    }
                    else
                        d[i] = -1;
                    /*printf("edge %d %d\n", i, tmpR);
                    paras->at(i)->print();
                    fflush(stdout);*/
                }
            }
        }
        else
        {
            if (spilledNodes.find(LA.ValueIdMap[paras->at(i)]) != spilledNodes.end())
            {
                // noting to do
            }
            else
            {
                Alloca *alloc = new Alloca(p_func->get_entryBB(), paras->at(i)->get_type(), 1);
                alloc->insertInstr(p_func->get_entryBB(), 0);
                nowPos++;
                paraMap[alloc] = paras->at(i);
                int tmpR = getReg(paras->at(i));
                Load *load = new Load(alloc, false, p_func->get_entryBB());
                load->insertInstr(p_func->get_entryBB(), nowPos++);
                if (tmpR >= Para_num)
                {
                    valueMapRegister[load] = tmpR;
                }
                else
                {
                    In[tmpR] = load;
                }
            }
        }
    }
    int n = Para_num;
    puts("");
    p_func->print();
    for (int i = 0; i < paras->size(); i++)
        printf("R %d\n", getReg(paras->at(i)));
    fflush(stdout);
    while (1)
    {
        bool flag = false;
        for (int i = 0; i < n; i++)
            if (In[i] != nullptr && d[i] >= 0)
                flag = true;
        if (!flag)
            break;
        flag = false;
        for (int i = 0; i < n; i++)
        {
            fflush(stdout);
            if (In[i] == nullptr)
                continue;
            if (d[i] == 0)
            {
                flag = true;
                if (dynamic_cast<Load *>(In[i]) == nullptr)
                {

                    Move *move = new Move(InstrutionEnum::Move, Register[i], In[i], p_func->get_entryBB());
                    move->insertInstr(p_func->get_entryBB(), nowPos++);
                    if (getReg(In[i]) < n)
                        d[getReg(In[i])]--;
                }
                else
                {
                    dynamic_cast<Instrution *>(In[i])->insertInstr(p_func->get_entryBB(), nowPos);
                    valueMapRegister[In[i]] = i;
                }
                d[i] = -1;
                break;
            }
        }
        if (!flag)
        {
            for (int i = 0; i < n; i++)
                if (d[i] > 0)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[12], Register[i], p_func->get_entryBB());
                    move->insertInstr(p_func->get_entryBB(), nowPos++);
                    assert(In[i] != nullptr);
                    move = new Move(InstrutionEnum::Move, Register[i], In[i], p_func->get_entryBB());
                    move->insertInstr(p_func->get_entryBB(), nowPos++);
                    assert(std::find(Register.begin(), Register.end(), In[i]) != Register.end());
                    d[std::find(Register.begin(), Register.end(), In[i]) - Register.begin()]--;
                    d[i] = -1;
                    for (int j = 0; j < n; j++)
                        if (d[j] >= 0 && In[j] == Register[i])
                        {
                            In[j] = Register[12];
                        }
                    flag = true;
                    break;
                }
        }
        assert(flag);
    }
}

void SSARegisterAlloc::ReSortForCall(Call *call)
{
    int nowPos = std::find(call->get_parent()->get_instrs()->begin(), call->get_parent()->get_instrs()->end(), call) - call->get_parent()->get_instrs()->begin();
    std::vector<int> d(Para_num, 0);
    std::vector<Value *> In(Para_num, nullptr);
    int cnt = 0;
    for (auto edge : *(call->get_value_list()))
    {
        Value *val = edge->get_val();
        if (dynamic_cast<Function *>(val) != nullptr)
            continue;
        int regFrom = getReg(val);
        assert(regFrom != -1 || dynamic_cast<Constant *>(val) != nullptr);
        if (regFrom == cnt)
        {
            d[cnt] = -1;
        }
        else
        {
            if (regFrom < Para_num && regFrom >= 0)
                d[regFrom]++;
            In[cnt] = val;
        }
        cnt++;
        if (cnt == Para_num)
            break;
    }
    while (1)
    {
        bool flag = false;
        for (int i = 0; i < cnt; i++)
            if (d[i] >= 0)
            {
                flag = true;
                break;
            }
        if (!flag)
            break;
        flag = false;
        for (int i = 0; i < cnt; i++)
            if (In[i] != nullptr && d[i] == 0)
            {
                int regFrom = getReg(In[i]);
                Move *move = new Move(InstrutionEnum::Move, Register[i], In[i], call->get_parent());
                if (firstMoveofCall.find(call) == firstMoveofCall.end())
                {
                    firstMoveofCall[call] = move;
                }
                move->insertInstr(move->get_parent(), nowPos++);
                if (regFrom < cnt && regFrom >= 0)
                    d[regFrom]--;
                d[i] = -1;
                flag = true;
                break;
            }
        if (!flag)
        {
            for (int i = 0; i < cnt; i++)
                if (d[i] > 0)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[12], Register[i], call->get_parent());
                    if (firstMoveofCall.find(call) == firstMoveofCall.end())
                    {
                        firstMoveofCall[call] = move;
                    }
                    move->insertInstr(move->get_parent(), nowPos++);
                    move = new Move(InstrutionEnum::Move, Register[i], In[i], call->get_parent());
                    move->insertInstr(move->get_parent(), nowPos++);
                    if (getReg(In[i]) < cnt)
                        d[getReg(In[i])]--;
                    for (int j = 0; j < cnt; j++)
                        if (In[j] == Register[i])
                            In[j] = Register[12];
                    d[i] = -1;
                    flag = true;
                    break;
                }
        }
        assert(flag);
    }
}

void SSARegisterAlloc::ReSortForPhi(BasicBlock *bb)
{
    if (bb->get_phinodes()->empty())
        return;
    int n = bb->get_phinodes()->size();
    for (auto it : *(bb->get_phinodes()->front()->get_valueMap()))
    {
        BasicBlock *bbFrom = it.first;
        std::vector<int> d(n, 0);
        std::vector<Value *> In(n, nullptr);
        std::vector<Value *> a;
        std::vector<int> b;
        std::unordered_map<int, int> Q;
        for (auto phi : *(bb->get_phinodes()))
        {
            a.push_back(phi->get_valueMap()->at(bbFrom)->get_val());
            b.push_back(getReg(phi));
            Q[getReg(phi)] = a.size() - 1;
        }
        assert(a.size() == n);
        for (int i = 0; i < n; i++)
        {
            In[i] = a[i];
            if (getReg(a[i]) == b[i])
            {
                d[i] = -1;
            }
            else
            {
                if (getReg(a[i]) >= 0 && Q.find(getReg(a[i])) != Q.end())
                    d[Q[getReg(a[i])]]++;
            }
        }
        while (1)
        {
            bool flag = false;
            for (int i = 0; i < n; i++)
                if (d[i] >= 0)
                {
                    flag = true;
                    break;
                }
            if (!flag)
                break;
            flag = false;
            for (int i = 0; i < n; i++)
                if (d[i] == 0)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[b[i]], In[i], bbFrom);
                    move->insertInstr(bbFrom, bbFrom->get_instrs()->size() - 1);
                    d[i] = -1;
                    if (getReg(a[i]) >= 0 && Q.find(getReg(a[i])) != Q.end())
                        d[Q[getReg(a[i])]]--;
                    flag = true;
                    break;
                }
            if (!flag)
            {
                for (int i = 0; i < n; i++)
                    if (d[i] > 0)
                    {
                        Move *move = new Move(InstrutionEnum::Move, Register[12], Register[b[i]], bbFrom);
                        move->insertInstr(bbFrom, bbFrom->get_instrs()->size() - 1);
                        move = new Move(InstrutionEnum::Move, Register[b[i]], In[i], bbFrom);
                        move->insertInstr(bbFrom, bbFrom->get_instrs()->size() - 1);
                        d[i] = -1;
                        if (getReg(a[i]) >= 0 && Q.find(getReg(a[i])) != Q.end())
                            d[Q[getReg(a[i])]]--;
                        for (int j = 0; j < n; j++)
                            if (In[j] == Register[b[i]])
                                In[j] = Register[12];
                        flag = true;
                        break;
                    }
            }
            assert(flag);
        }
    }
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
            // if (dynamic_cast<Call *>(*it) != nullptr)
            //     continue;
            int cnt = 0;
            for (auto edge : *((*it)->get_value_list()))
            {
                Value *use = edge->get_val();
                if (dynamic_cast<Function *>(use) != nullptr)
                    continue;
                if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                    live.insert(LA.ValueIdMap.at(use));
                cnt++;
                if (cnt == Para_num)
                    break;
            }
        }
        for (auto it : *(bb->get_phinodes()))
        {
            int def = LA.ValueIdMap.at(it);
            // assert(live.find(def) != live.end());
            if (live.find(def) != live.end())
            {
                live.erase(def);
                for (auto l : live)
                    AddEdge(l, def);
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
                    {
                        AddEdge(l, def);
                    }
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
    int N = n + 1;
    std::vector<int> next(n + N + 1), pre(n + N + 1), w(n + 1), q(n + 1), dy(n + 1);
    for (int i = 1; i <= n; i++)
    {
        color[id_R[i]] = -1;
        pre[next[i] = next[N + w[i]]] = i;
        next[pre[i] = N + w[i]] = i;
    }
    int now = 0;
    for (int k = n; k; --k, ++now)
    {
        while (!next[N + now])
            --now;
        int x = next[N + now];
        std::unordered_set<int> color_set;
        for (auto tmp : G[id_R[x]])
            if (color[tmp] != -1)
                color_set.insert(color[tmp]);
        for (int i = 0; i < K_R; i++)
            if (color_set.find(i) == color_set.end())
            {
                color[id_R[x]] = i;
                break;
            }
        if (color[id_R[x]] == -1)
        {
            assert(0);
        }
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
                pre[next[y] = next[N + w[y]]] = y;
                next[pre[y] = N + w[y]] = y;
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
    int N = n + 1;
    std::vector<int> next(n + N + 1), pre(n + N + 1), w(n + 1), q(n + 1), dy(n + 1);
    for (int i = 1; i <= n; i++)
    {
        color[id_S[i]] = -1;
        pre[next[i] = next[N + w[i]]] = i;
        next[pre[i] = N + w[i]] = i;
    }
    int now = 0;
    for (int k = n; k; --k, ++now)
    {
        while (!next[N + now])
            --now;
        int x = next[N + now];
        std::unordered_set<int> color_set;
        for (auto tmp : G[id_S[x]])
            if (color[tmp] != -1)
                color_set.insert(color[tmp]);
        for (int i = 0; i < K_R; i++)
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
                pre[next[y] = next[N + w[y]]] = y;
                next[pre[y] = N + w[y]] = y;
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
    for (auto phi : *(bb->get_phinodes()))
        if (LA.is_float[LA.ValueIdMap[phi]] == 0)
            I_B.insert(std::make_pair(LA.InDis[bb][LA.ValueIdMap[phi]], LA.ValueIdMap[phi]));
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
        int cnt = 0;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            cnt++;
            if (cnt <= Para_num)
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 0)
                    ops.insert(LA.ValueIdMap.at(edge->get_val()));
            }
            else
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 0)
                {
                    spilledNodes.insert(LA.ValueIdMap.at(edge->get_val()));
                    spillUserPhi(LA.ValueIdMap.at(edge->get_val()));
                }
            }
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
                    assert(0);
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
    for (auto phi : *(bb->get_phinodes()))
        if (LA.is_float[LA.ValueIdMap[phi]] == 1)
            I_B.insert(std::make_pair(LA.InDis[bb][LA.ValueIdMap[phi]], LA.ValueIdMap[phi]));
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
        int cnt = 0;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            cnt++;
            if (cnt <= Para_num)
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 1)
                    ops.insert(LA.ValueIdMap.at(edge->get_val()));
            }
            else
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_float[LA.ValueIdMap.at(edge->get_val())] == 1)
                {
                    spilledNodes.insert(LA.ValueIdMap.at(edge->get_val()));
                    spillUserPhi(LA.ValueIdMap.at(edge->get_val()));
                }
            }
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
    for (auto v : spilledNodes)
    {
        Alloca *alloc = new Alloca(ebb, LA.Vals[v]->get_type(), 1);
        assert(alloc->get_type()->get_type() == TypeEnum::Ptr);
        alloc->insertInstr(ebb, 0);
        allocMap[v] = alloc;
        if (dynamic_cast<Param *>(LA.Vals[v]) != nullptr)
            if (p_func->get_params()->size() > Para_num && std::find(p_func->get_params()->begin(), p_func->get_params()->begin() + Para_num, LA.Vals[v]) == p_func->get_params()->begin() + Para_num)
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
                    store->insertInstr(store->get_parent(), insNum - 2);
                }
                else
                {
                    store->insertInstr(store->get_parent(), insNum - 1);
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
        /*else if (para != nullptr && (p_func->get_params()->size() <= Para_num || std::find(p_func->get_params()->begin(), p_func->get_params()->begin() + Para_num, para) != p_func->get_params()->begin() + Para_num))
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
            bool flag = true;
            if (dynamic_cast<Call *>(use) != nullptr && use->get_value_list()->size() > Para_num + 1)
            {
                flag = false;
                for (int i = 1; i <= Para_num; i++)
                    if (use->get_value_list()->at(i)->get_val() == val)
                    {
                        flag = true;
                        break;
                    }
            }
            auto insList = use->get_parent()->get_instrs();
            if (std::find(insList->begin(), insList->end(), use) != insList->end())
            {
                if (flag)
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
                else
                {
                    it->set_val(allocMap[v]);
                }
            }
        }
        Edge *tmp_edge = nullptr;
        for (auto edge : *(val->get_user_list()))
        {
            if (edge->get_val() == val && dynamic_cast<PHINode *>(edge->get_user()) == nullptr)
            {
                tmp_edge = edge;
                break;
            }
        }
        val->get_user_list()->clear();
        if (tmp_edge != nullptr)
            val->get_user_list()->push_back(tmp_edge);
        PHINode *phi = dynamic_cast<PHINode *>(val);
        if (phi != nullptr)
            phi->drop();
    }
}