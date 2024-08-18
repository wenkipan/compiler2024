#include "../../include/ir_opt/SSARegisterAlloc.hpp"
#include "ir/Instrution.hpp"
#include <any>
#include <vector>

std::vector<int> SSARegisterAlloc::regsStillAliveAfterCall(Call *call)
{
    auto vrs = callLiveVreg[call];
    std::vector<int> ret;
    for (auto it : vrs)
    {
        if (spilledVals.find(it) != spilledVals.end())
            continue;
        int c = color[LA.ValueIdMap[it]];
        if (LA.is_float[LA.ValueIdMap[it]])
        {
            if (c <= 15 && c >= 0)
                ret.push_back(c + 16);
        }
        else if (LA.is_vector[LA.ValueIdMap[it]])
        {
            ret.push_back(56 + c);
        }
        else
        {
            if (c <= 3)
                ret.push_back(c);
            else if (c == 12)
                ret.push_back(14);
        }
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
        if (c < 0 || c > 31)
            assert(0);
        return 16 + c;
    }
    else if (LA.is_vector[x])
    {
        if (c < 0 || c > 7)
            assert(0);
        return 56 + c;
    }
    else
    {
        if (c <= 11)
            return c;
        else if (c == 12)
            return 14;
        else
            assert(0);
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

GlobalVariable *SSARegisterAlloc::whichGV(Alloca *alloc)
{
    if (gvMap.find(alloc) == gvMap.end())
        return nullptr;
    else
        return gvMap[alloc];
}

void SSARegisterAlloc::ReplaceNullToTmp(Function *p_func)
{
    for (auto bb : *(p_func->get_blocks()))
        for (auto ins : *(bb->get_instrs()))
        {
            if (is_a<Move>(ins))
            {
                for (auto it : *(ins->get_value_list()))
                    if (it->get_val() == nullptr)
                        it->set_val(Register[12]);
            }
            if (is_a<Store>(ins))
            {
                for (auto it : *(ins->get_value_list()))
                    if (it->get_val() == nullptr)
                        it->set_val(Register[12]);
            }
        }
}

void SSARegisterAlloc::run(Function *p_func)
{
    // GVtoA gvtoa;
    // gvtoa.run(p_func);
    p_func->ResetID(false);
    LA.run(p_func);
    AddTripleX(p_func);

    AddBB(p_func);
    p_func->ResetID(false);
    LA.run(p_func);

    vregNum = LA.Vals.size();
    Spill(p_func);
    for (auto it : spilledNodes)
        spilledVals.insert(LA.Vals[it]);

    RewriteProgram(p_func);
    MakeGraph(p_func);
    AnalysisTriple(p_func);
    AssignColor_R(p_func);
    AssignColor_Q(p_func);
    AssignColor_S(p_func);

    Register.resize(64);
    for (int i = 0; i < 64; i++)
    {
        if (i < 48)
            Register[i] = new Value(i < 16 ? TypeEnum::I32 : TypeEnum::F32);
        else if (i < 56)
            Register[i] = nullptr;
        else
            Register[i] = new Value(TypeEnum::VecI32);
        if (Register[i] != nullptr)
            p_func->value_pushBack(Register[i]);
    }

    ReplaceNullToTmp(p_func);
    DoTripleX(p_func);

    // for (auto bb : *(p_func->get_blocks()))
    // ReLoad(bb);

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
}

void SSARegisterAlloc::ReSortForPara(Function *p_func)
{
    std::unordered_map<int, int> Regs, RegsID;
    for (int i = 0; i < 4; i++)
    {
        Regs[i] = i;
        RegsID[i] = i;
    }
    for (int i = 16; i < 32; i++)
    {
        Regs[i - 12] = i;
        RegsID[i] = i - 12;
    }
    int nowPos = 0;
    BasicBlock *ebb = p_func->get_entryBB();
    for (auto it : *(ebb->get_instrs()))
        if (is_a<Alloca>(it))
            nowPos++;
        else
            break;
    auto Paras = p_func->get_params();
    int cnt_R = 0, cnt_S = 0;
    std::vector<int> d(Para_Sum, 0);
    std::vector<Value *> In(Para_Sum, nullptr);
    for (auto para : *Paras)
    {
        if (para->get_type()->get_type() == TypeEnum::F32)
        {
            cnt_S++;
            if (cnt_S <= Para_S)
            {
                if (spilledVals.find(para) == spilledVals.end())
                {
                    int reg = getReg(para);
                    int reg_now = cnt_S + 15;
                    if (reg == reg_now)
                    {
                        d[RegsID[reg]] = -1;
                        continue;
                    }
                    if (RegsID.find(reg) != RegsID.end()) // reg_now -> reg;
                    {
                        d[RegsID[reg_now]]++;
                        In[RegsID[reg]] = Register[reg_now];
                    }
                    else
                    {
                        Move *move = new Move(InstrutionEnum::Move, Register[reg], Register[reg_now], ebb);
                        move->insertInstr(ebb, nowPos++);
                    }
                }
                else
                {
                    int reg_now = cnt_S + 15;
                    Value *val_tmp = new Value(para->get_type());
                    p_func->value_pushBack(val_tmp);
                    valueMapRegister[val_tmp] = reg_now;
                    Store *store = new Store(allocMap[para], val_tmp, false, ebb);
                    store->insertInstr(ebb, nowPos++);
                }
            }
            else
            {
                if (spilledVals.find(para) == spilledVals.end())
                {
                    int reg = getReg(para);
                    Alloca *alloc = new Alloca(ebb, para->get_type(), 1);
                    allocMap[para] = alloc;
                    paraMap[alloc] = para;
                    ebb->get_instrs()->pop_back();
                    Load *load = new Load(alloc, false, ebb);
                    load->insertInstr(ebb, nowPos++);
                    valueMapRegister[load] = reg;
                    if (RegsID.find(reg) != RegsID.end())
                        In[RegsID[reg]] = load;
                }
            }
        }
        else if (para->get_type()->get_type() == TypeEnum::VecI32)
        {
            assert(0);
        }
        else
        {
            cnt_R++;
            if (cnt_R <= Para_R)
            {
                if (spilledVals.find(para) == spilledVals.end())
                {
                    int reg = getReg(para);
                    int reg_now = cnt_R - 1;
                    if (reg == reg_now)
                    {
                        d[RegsID[reg]] = -1;
                        continue;
                    }
                    if (RegsID.find(reg) != RegsID.end()) // reg_now -> reg;
                    {
                        d[RegsID[reg_now]]++;
                        In[RegsID[reg]] = Register[reg_now];
                    }
                    else
                    {
                        Move *move = new Move(InstrutionEnum::Move, Register[reg], Register[reg_now], ebb);
                        move->insertInstr(ebb, nowPos++);
                    }
                }
                else
                {
                    int reg_now = cnt_R - 1;
                    Value *val_tmp = new Value(para->get_type());
                    p_func->value_pushBack(val_tmp);
                    valueMapRegister[val_tmp] = reg_now;
                    Store *store = new Store(allocMap[para], val_tmp, false, ebb);
                    store->insertInstr(ebb, nowPos++);
                }
            }
            else
            {
                if (spilledVals.find(para) == spilledVals.end())
                {
                    int reg = getReg(para);
                    Alloca *alloc = new Alloca(ebb, para->get_type(), 1);
                    allocMap[para] = alloc;
                    paraMap[alloc] = para;
                    ebb->get_instrs()->pop_back();
                    Load *load = new Load(alloc, false, ebb);
                    load->insertInstr(ebb, nowPos++);
                    valueMapRegister[load] = reg;
                    if (RegsID.find(reg) != RegsID.end())
                        In[RegsID[reg]] = load;
                }
            }
        }
    }
    while (1)
    {
        bool flag = false;
        for (int i = 0; i < Para_Sum; i++)
            if (In[i] != nullptr && d[i] >= 0)
            {
                flag = true;
                break;
            }
        if (!flag)
            break;
        flag = false;
        for (int i = 0; i < Para_Sum; i++)
            if (In[i] != nullptr && d[i] == 0)
            {
                if (is_a<Load>(In[i]))
                    dynamic_cast<Load *>(In[i])->insertInstr(ebb, nowPos);
                else
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[Regs[i]], In[i], ebb);
                    move->insertInstr(ebb, nowPos++);
                    if (RegsID.find(getReg(In[i])) != RegsID.end())
                        d[RegsID[getReg(In[i])]]--;
                }
                flag = true;
                d[i] = -1;
                break;
            }
        if (!flag)
        {
            for (int i = 0; i < Para_Sum; i++)
                if (d[i] > 0)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[12], Register[Regs[i]], ebb);
                    move->insertInstr(ebb, nowPos++);
                    if (is_a<Load>(In[i]))
                        dynamic_cast<Load *>(In[i])->insertInstr(ebb, nowPos);
                    else
                    {
                        Move *move = new Move(InstrutionEnum::Move, Register[Regs[i]], In[i], ebb);
                        move->insertInstr(ebb, nowPos++);
                        if (RegsID.find(getReg(In[i])) != RegsID.end())
                            d[RegsID[getReg(In[i])]]--;
                    }
                    for (int j = 0; j < Para_Sum; j++)
                        if (d[j] >= 0 && In[j] == Register[Regs[i]])
                            In[j] = Register[12];
                    flag = true;
                    d[i] = -1;
                    break;
                }
        }
        assert(flag);
    }
}

void SSARegisterAlloc::ReSortForCall(Call *call)
{
    int nowPos = std::find(call->get_parent()->get_instrs()->begin(), call->get_parent()->get_instrs()->end(), call) - call->get_parent()->get_instrs()->begin();
    std::vector<int> d(Para_Sum, 0);
    std::vector<Value *> In(Para_Sum, nullptr);
    std::unordered_map<int, int> Regs, RegsID;
    for (int i = 0; i < 4; i++)
    {
        Regs[i] = i;
        RegsID[i] = i;
    }
    for (int i = 16; i < 32; i++)
    {
        Regs[i - 12] = i;
        RegsID[i] = i - 12;
    }
    int cnt_R = 0, cnt_S = 0;
    for (auto edge : *(call->get_value_list()))
    {
        Value *op = edge->get_val();
        if (is_a<Function>(op))
            continue;
        if (op->get_type()->get_type() == TypeEnum::F32)
        {
            cnt_S++;
            if (cnt_S <= Para_S)
            {
                int Reg = 15 + cnt_S;
                int Reg_now = getReg(op);
                if (Reg == Reg_now)
                {
                    d[RegsID[Reg]] = -1;
                    continue;
                }
                if (Reg_now >= 0)
                {
                    In[RegsID[Reg]] = Register[Reg_now];
                    if (RegsID.find(Reg_now) != RegsID.end())
                        d[RegsID[Reg_now]]++;
                }
                else
                    In[RegsID[Reg]] = op;
            }
        }
        else if (op->get_type()->get_type() == TypeEnum::VecI32)
        {
            assert(0);
        }
        else
        {
            if (is_a<Alloca>(op) && isSpill(dynamic_cast<Alloca *>(op)))
                continue;
            cnt_R++;
            if (cnt_R <= Para_R)
            {
                int Reg = cnt_R - 1;
                int Reg_now = getReg(op);
                if (Reg == Reg_now)
                {
                    d[RegsID[Reg]] = -1;
                    continue;
                }
                if (Reg_now >= 0)
                {
                    In[RegsID[Reg]] = Register[Reg_now];
                    if (RegsID.find(Reg_now) != RegsID.end())
                        d[RegsID[Reg_now]]++;
                }
                else
                    In[RegsID[Reg]] = op;
            }
        }
    }
    BasicBlock *bb = call->get_BB();
    while (1)
    {
        bool flag = false;
        for (int i = 0; i < Para_Sum; i++)
            if (In[i] != nullptr && d[i] >= 0)
            {
                flag = true;
                break;
            }
        if (!flag)
            break;
        flag = false;
        for (int i = 0; i < Para_Sum; i++)
            if (In[i] != nullptr && d[i] == 0)
            {
                Move *move = new Move(InstrutionEnum::Move, Register[Regs[i]], In[i], bb);
                move->insertInstr(bb, nowPos++);
                if (firstMoveofCall.find(call) == firstMoveofCall.end())
                    firstMoveofCall[call] = move;
                if (RegsID.find(getReg(In[i])) != RegsID.end())
                    d[RegsID[getReg(In[i])]]--;
                flag = true;
                d[i] = -1;
                break;
            }
        if (!flag)
        {
            for (int i = 0; i < Para_Sum; i++)
                if (d[i] > 0)
                {
                    Move *move = new Move(InstrutionEnum::Move, Register[12], Register[Regs[i]], bb);
                    move->insertInstr(bb, nowPos++);
                    if (firstMoveofCall.find(call) == firstMoveofCall.end())
                        firstMoveofCall[call] = move;
                    // if (is_a<Load>(In[i]))
                    //     dynamic_cast<Load *>(In[i])->insertInstr(bb, nowPos);
                    // else
                    {
                        Move *move = new Move(InstrutionEnum::Move, Register[Regs[i]], In[i], bb);
                        move->insertInstr(bb, nowPos++);
                        if (firstMoveofCall.find(call) == firstMoveofCall.end())
                            firstMoveofCall[call] = move;
                        if (RegsID.find(getReg(In[i])) != RegsID.end())
                            d[RegsID[getReg(In[i])]]--;
                    }
                    for (int j = 0; j < Para_Sum; j++)
                        if (d[j] >= 0 && In[j] == Register[Regs[i]])
                            In[j] = Register[12];
                    flag = true;
                    d[i] = -1;
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
                            if (d[j] >= 0 && getReg(In[j]) == b[i])
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
    if (x == y || LA.is_float[x] != LA.is_float[y] || LA.is_vector[x] != LA.is_vector[y])
        return;
    G_set[LA.Vals[x]].insert(LA.Vals[y]);
    G_set[LA.Vals[y]].insert(LA.Vals[x]);
    G[x].push_back(y);
    G[y].push_back(x);
}

void SSARegisterAlloc::MakeGraph(Function *p_func)
{
    p_func->ResetID(false);
    LA.run(p_func);
    vregNum = LA.Vals.size();
    G.clear();
    G_set.clear();
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
            int cnt_R = 0, cnt_S = 0, cnt_Q = 0;
            for (auto edge : *((*it)->get_value_list()))
            {
                Value *use = edge->get_val();
                if (dynamic_cast<Function *>(use) != nullptr)
                    continue;
                if (use->get_type()->get_type() == TypeEnum::F32)
                {
                    cnt_S++;
                    if (cnt_S > Para_S)
                        continue;
                }
                else if (use->get_type()->get_type() == TypeEnum::VecI32)
                {
                    cnt_Q++;
                }
                else
                {
                    cnt_R++;
                    if (cnt_R > Para_R)
                        continue;
                }
                if (LA.ValueIdMap.find(use) != LA.ValueIdMap.end())
                    live.insert(LA.ValueIdMap.at(use));
            }
            assert(cnt_Q >= 0);
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

void SSARegisterAlloc::graphBuilder(Function *p_func)
{
    MakeGraph(p_func);
}

void SSARegisterAlloc::AssignColor_R(Function *p_func)
{
    int n = 0;
    std::vector<int> id_R;
    std::unordered_map<int, int> id_R_map;
    id_R.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_int[i] == 1)
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
        int root = LA.get_father(id_R[x]);
        if (color_root.find(root) != color_root.end())
        {
            int best = color_root.at(root);
            if (color_set.find(best) == color_set.end())
            {
                color[id_R[x]] = best;
            }
        }
        if (color[id_R[x]] == -1)
            for (int i = 3; i >= 0; i--)
                if (color_set.find(i) == color_set.end())
                {
                    color[id_R[x]] = i;
                    break;
                }
        if (color[id_R[x]] == -1)
            for (int i = 4; i < K_R; i++)
                if (color_set.find(i) == color_set.end())
                {
                    color[id_R[x]] = i;
                    break;
                }
        if (color[id_R[x]] == -1)
        {
            assert(0);
        }
        if (color_root.find(root) == color_root.end())
            color_root.insert(std::make_pair(root, color[id_R[x]]));
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

void SSARegisterAlloc::AssignColor_Q(Function *p_func)
{
    int n = 0;
    std::vector<int> id_Q;
    std::unordered_map<int, int> id_Q_map;
    id_Q.push_back(-1);
    for (int i = 0; i < vregNum; i++)
        if (LA.is_vector[i] == 1)
        {
            id_Q.push_back(i);
            n++;
            id_Q_map[i] = n;
        }
    int N = n + 1;
    std::vector<int> next(n + N + 1), pre(n + N + 1), w(n + 1), q(n + 1), dy(n + 1);
    for (int i = 1; i <= n; i++)
    {
        color[id_Q[i]] = -1;
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
        for (auto tmp : G[id_Q[x]])
            if (color[tmp] != -1)
                color_set.insert(color[tmp]);
        int root = LA.get_father(id_Q[x]);
        if (color_root.find(root) != color_root.end())
        {
            int best = color_root.at(root);
            if (color_set.find(best) == color_set.end())
            {
                color[id_Q[x]] = best;
            }
        }
        if (color[id_Q[x]] == -1)
            for (int i = 0; i < 8; i++)
                if (color_set.find(i) == color_set.end())
                {
                    color[id_Q[x]] = i;
                    break;
                }
        if (color[id_Q[x]] == -1)
        {
            assert(0);
        }
        if (color_root.find(root) == color_root.end())
            color_root.insert(std::make_pair(root, color[id_Q[x]]));
        pre[next[x]] = pre[x];
        next[pre[x]] = next[x];
        q[k] = x;
        dy[x] = k;
        for (auto tmp : G[id_Q[x]])
        {
            int y = id_Q_map[tmp];
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
        int root = LA.get_father(id_S[x]);
        if (color_root.find(root) != color_root.end())
        {
            int best = color_root.at(root);
            if (color_set.find(best) == color_set.end())
            {
                color[id_S[x]] = best;
            }
        }
        if (color[id_S[x]] == -1)
            for (int i = 15; i >= 0; i--)
                if (color_set.find(i) == color_set.end())
                {
                    color[id_S[x]] = i;
                    break;
                }
        if (color[id_S[x]] == -1)
            for (int i = 16; i < K_S; i++)
                if (color_set.find(i) == color_set.end())
                {
                    color[id_S[x]] = i;
                    break;
                }
        if (color[id_S[x]] == -1)
        {
            assert(0);
        }
        if (color_root.find(root) == color_root.end())
            color_root.insert(std::make_pair(root, color[id_S[x]]));
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

void SSARegisterAlloc::ReLoad(BasicBlock *bb)
{
    if (spilledVals.empty())
        return;
    std::unordered_map<int, Value *> regState;
    std::unordered_set<Value *> InReg;
    std::unordered_map<Alloca *, std::queue<Value *>> Q;
    std::vector<int> allR, allS;
    for (int i = 8; i <= 27; i++)
    {
        allR.push_back(i);
        allS.push_back(i + 32);
    }
    for (int i = 0; i < vregNum; i++)
        if (LA.InSet[bb].at(i))
        {
            regState[getReg(LA.Vals[i])] = LA.Vals[i];
            InReg.insert(LA.Vals[i]);
        }
    for (auto phi : *(bb->get_phinodes()))
    {
        regState[getReg(phi)] = phi;
        InReg.insert(phi);
    }
    std::vector<Instrution *> dropList;
    for (auto ins : *(bb->get_instrs()))
    {
        int cnt_R = 0, cnt_S = 0;
        std::vector<Value *> uses;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            Value *val = edge->get_val();
            if (val->get_type()->get_type() != TypeEnum::F32)
            {
                cnt_R++;
                if (cnt_R > Para_R)
                    continue;
                uses.push_back(val);
            }
            else if (val->get_type()->get_type() == TypeEnum::F32)
            {
                cnt_S++;
                if (cnt_S > Para_S)
                    continue;
                uses.push_back(val);
            }
        }
        for (auto val : uses)
        {
            if (is_a<Load>(val) && isSpill(dynamic_cast<Alloca *>(dynamic_cast<Load *>(val)->get_addr())))
            {
                if (InReg.find(val) == InReg.end())
                    continue;
                Alloca *alloc = dynamic_cast<Alloca *>(dynamic_cast<Load *>(val)->get_addr());
                while (1)
                {
                    assert(!Q[alloc].empty());
                    if (InReg.find(Q[alloc].front()) == InReg.end())
                        Q[alloc].pop();
                    else
                        break;
                }
                if (val == Q[alloc].front())
                    continue;
                InReg.erase(regState[getReg(val)]);
                regState[getReg(val)] = nullptr;
                for (auto edge : *(val->get_user_list()))
                    edge->set_val(Q[alloc].front());
                val->get_user_list()->clear();
                dropList.push_back(dynamic_cast<Instrution *>(val));
            }
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end())
        {
            if (regState[getReg(ins)] != nullptr)
            {
                InReg.erase(regState[getReg(ins)]);
            }
            InReg.insert(ins);
            regState[getReg(ins)] = ins;
            if (is_a<Load>(ins) && isSpill(dynamic_cast<Alloca *>(dynamic_cast<Load *>(ins)->get_addr())))
                Q[dynamic_cast<Alloca *>(dynamic_cast<Load *>(ins)->get_addr())].push(dynamic_cast<Load *>(ins));
            else if (spilledVals.find(ins) != spilledVals.end())
            {
                Q[allocMap[ins]].push(ins);
            }
        }
        if (is_a<Call>(ins))
        {
            for (int i = 0; i < 4; i++)
                if (regState[i] != nullptr)
                {
                    InReg.erase(regState[i]);
                    regState[i] = nullptr;
                }
            for (int i = 16; i < 32; i++)
                if (regState[i] != nullptr)
                {
                    InReg.erase(regState[i]);
                    regState[i] = nullptr;
                }
        }
    }
    for (auto ins : dropList)
        ins->drop();
}

void SSARegisterAlloc::SpillBB_R(BasicBlock *bb)
{
    std::set<std::pair<int, int>> I_B;
    for (int i = 0; i < vregNum; i++)
        if (LA.InSet[bb].at(i) && LA.is_int[i] == 1)
            I_B.insert(std::make_pair(LA.InDis[bb][i], i));
    for (auto phi : *(bb->get_phinodes()))
        if (LA.is_int[LA.ValueIdMap[phi]] == 1)
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
        int cnt_R = 0;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            if (edge->get_val()->get_type()->get_type() != TypeEnum::F32 && edge->get_val()->get_type()->get_type() != TypeEnum::VecI32)
                cnt_R++;
            else
                continue;
            if (cnt_R <= Para_R)
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_int[LA.ValueIdMap.at(edge->get_val())] == 1)
                    ops.insert(LA.ValueIdMap.at(edge->get_val()));
            }
            else
            {
                if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_int[LA.ValueIdMap.at(edge->get_val())] == 1)
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
        std::set<std::pair<int, int>> del_reg;
        for (auto reg : Regs)
        {
            if (spilledNodes.find(reg.second) != spilledNodes.end())
            {
                inReg.erase(reg.second);
                // Regs.erase(reg);
                del_reg.insert(reg);
            }
            if (LA.OutSet[bb].at(reg.second))
            {
                continue;
            }
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
                // Regs.erase(reg);
                del_reg.insert(reg);
            }
        }
        for (auto reg : del_reg)
            Regs.erase(reg);
        if (dynamic_cast<Call *>(ins) != nullptr)
        {
            for (auto reg : Regs)
            {
                callLiveVreg[dynamic_cast<Call *>(ins)].push_back(LA.Vals[reg.second]);
            }
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_int[LA.ValueIdMap.at(ins)] == 1)
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

void SSARegisterAlloc::SpillBB_Q(BasicBlock *bb)
{
    std::set<std::pair<int, int>> I_B;
    for (int i = 0; i < vregNum; i++)
        if (LA.InSet[bb].at(i) && LA.is_vector[i] == 1)
            I_B.insert(std::make_pair(LA.InDis[bb][i], i));
    for (auto phi : *(bb->get_phinodes()))
        if (LA.is_vector[LA.ValueIdMap[phi]] == 1)
            I_B.insert(std::make_pair(LA.InDis[bb][LA.ValueIdMap[phi]], LA.ValueIdMap[phi]));
    std::set<std::pair<int, int>> Regs;
    std::unordered_set<int> inReg;
    for (auto it : I_B)
    {
        if (Regs.size() < K_Q)
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
        int cnt_Q = 0;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            if (edge->get_val()->get_type()->get_type() == TypeEnum::VecI32)
                cnt_Q++;
            else
                continue;
            assert(!is_a<Call>(ins));
            if (LA.ValueIdMap.find(edge->get_val()) != LA.ValueIdMap.end() && LA.is_vector[LA.ValueIdMap.at(edge->get_val())] == 1)
                ops.insert(LA.ValueIdMap.at(edge->get_val()));
        }
        for (auto op : ops)
        {
            if (inReg.find(op) == inReg.end())
            {
                if (Regs.size() == K_Q)
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
                if (Regs.size() == K_Q)
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
        std::set<std::pair<int, int>> del_reg;
        for (auto reg : Regs)
        {
            if (spilledNodes.find(reg.second) != spilledNodes.end())
            {
                inReg.erase(reg.second);
                // Regs.erase(reg);
                del_reg.insert(reg);
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
                // Regs.erase(reg);
                del_reg.insert(reg);
            }
        }
        assert(cnt_Q >= 0);
        for (auto reg : del_reg)
            Regs.erase(reg);
        if (dynamic_cast<Call *>(ins) != nullptr)
        {
            for (auto reg : Regs)
                callLiveVreg[dynamic_cast<Call *>(ins)].push_back(LA.Vals[reg.second]);
        }
        if (LA.ValueIdMap.find(ins) != LA.ValueIdMap.end() && LA.is_vector[LA.ValueIdMap.at(ins)] == 1)
        {
            int result = LA.ValueIdMap.at(ins);
            if (Regs.size() == K_Q)
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
        int cnt_S = 0;
        for (auto edge : *(ins->get_value_list()))
        {
            if (dynamic_cast<Function *>(edge->get_val()) != nullptr)
                continue;
            if (edge->get_val()->get_type()->get_type() == TypeEnum::F32)
                cnt_S++;
            else
                continue;
            if (cnt_S <= Para_S)
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
        std::set<std::pair<int, int>> del_reg;
        for (auto reg : Regs)
        {
            if (spilledNodes.find(reg.second) != spilledNodes.end())
            {
                inReg.erase(reg.second);
                // Regs.erase(reg);
                del_reg.insert(reg);
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
                // Regs.erase(reg);
                del_reg.insert(reg);
            }
        }
        for (auto reg : del_reg)
            Regs.erase(reg);
        if (dynamic_cast<Call *>(ins) != nullptr)
        {
            for (auto reg : Regs)
                callLiveVreg[dynamic_cast<Call *>(ins)].push_back(LA.Vals[reg.second]);
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
        SpillBB_Q(bb);
        SpillBB_S(bb);
    }
}

void SSARegisterAlloc::RewriteProgram(Function *p_func)
{
    BasicBlock *ebb = p_func->get_entryBB();
    for (auto v : spilledNodes)
    {
        Alloca *alloc = new Alloca(ebb, LA.Vals[v]->get_type(), 1);
        spillAllocs.insert(alloc);
        spillAllocMap[alloc] = LA.Vals[v];
        assert(alloc->get_type()->get_type() == TypeEnum::Ptr);
        alloc->insertInstr(ebb, 0);
        allocMap[LA.Vals[v]] = alloc;
        if (dynamic_cast<Param *>(LA.Vals[v]) != nullptr)
        {
            bool flag = false;
            if (LA.Vals[v]->get_type()->get_type() == TypeEnum::F32)
            {
                int rk = 0;
                for (auto it : *(p_func->get_params()))
                    if (it->get_type()->get_type() == TypeEnum::F32)
                    {
                        rk++;
                        if (it == LA.Vals[v])
                            break;
                    }
                if (rk <= Para_S)
                    flag = true;
            }
            else if (LA.Vals[v]->get_type()->get_type() == TypeEnum::VecI32)
            {
                assert(0);
            }
            else
            {
                int rk = 0;
                for (auto it : *(p_func->get_params()))
                    if (it->get_type()->get_type() != TypeEnum::F32 && it->get_type()->get_type() != TypeEnum::VecI32)
                    {
                        rk++;
                        if (it == LA.Vals[v])
                            break;
                    }
                if (rk <= Para_R)
                    flag = true;
            }
            if (!flag)
            {
                paraMap[alloc] = dynamic_cast<Param *>(LA.Vals[v]);
                alloc->insertInstr(alloc->get_BB(), alloc->get_BB()->get_instrs()->size());
                alloc->get_BB()->get_instrs()->pop_back();
            }
        }
        if (dynamic_cast<Assign *>(LA.Vals[v]) != nullptr)
        {
            Assign *assign = dynamic_cast<Assign *>(LA.Vals[v]);
            GlobalVariable *val = dynamic_cast<GlobalVariable *>(assign->get_value_list()->front()->get_val());
            if (val != nullptr)
            {
                gvMap[alloc] = val;
                alloc->insertInstr(alloc->get_BB(), alloc->get_BB()->get_instrs()->size());
                alloc->get_BB()->get_instrs()->pop_back();
            }
        }
    }

    for (auto bb : *(p_func->get_blocks()))
    {
        std::vector<PHINode *> spilledPhis;
        for (auto phi : *(bb->get_phinodes()))
            if (spilledVals.find(phi) != spilledVals.end())
                spilledPhis.push_back(phi);
        if (spilledPhis.empty())
            continue;
        int n = spilledPhis.size();
        for (auto it : *((spilledPhis.front()->get_valueMap())))
        {
            BasicBlock *fromBB = it.first;
            std::unordered_map<Value *, int> d;
            std::vector<Value *> In(n, nullptr);
            for (int i = 0; i < n; i++)
            {
                In[i] = spilledPhis[i]->get_valueMap()->at(fromBB)->get_val();
                if (is_a<PHINode>(In[i]) && dynamic_cast<PHINode *>(In[i])->get_parent() == bb)
                    d[In[i]]++;
            }
            while (1)
            {
                bool flag = false;
                for (int i = 0; i < n; i++)
                    if (d[spilledPhis[i]] >= 0)
                    {
                        flag = true;
                        break;
                    }
                if (!flag)
                    break;
                flag = false;
                for (int i = 0; i < n; i++)
                    if (d[spilledPhis[i]] == 0)
                    {
                        Store *store = new Store(allocMap[spilledPhis[i]], In[i], false, fromBB);
                        int insNum = store->get_parent()->get_instrutions()->size();
                        if (insNum >= 2 && dynamic_cast<Branch *>(store->get_parent()->get_instrutions()->at(insNum - 2)) != nullptr)
                        {
                            store->insertInstr(store->get_parent(), insNum - 2);
                        }
                        else
                        {
                            store->insertInstr(store->get_parent(), insNum - 1);
                        }
                        if (is_a<PHINode>(In[i]) && dynamic_cast<PHINode *>(In[i])->get_parent() == bb)
                            d[In[i]]--;
                        d[spilledPhis[i]] = -1;
                        flag = true;
                        break;
                    }
                if (!flag)
                    for (int i = 0; i < n; i++)
                        if (d[spilledPhis[i]] > 0)
                        {
                            Move *move = new Move(InstrutionEnum::Move, nullptr, In[i], fromBB);
                            int insNum = move->get_parent()->get_instrutions()->size();
                            if (insNum >= 2 && dynamic_cast<Branch *>(move->get_parent()->get_instrutions()->at(insNum - 2)) != nullptr)
                            {
                                move->insertInstr(move->get_parent(), insNum - 2);
                            }
                            else
                            {
                                move->insertInstr(move->get_parent(), insNum - 1);
                            }
                            Store *store = new Store(allocMap[spilledPhis[i]], In[i], false, fromBB);
                            insNum = store->get_parent()->get_instrutions()->size();
                            if (insNum >= 2 && dynamic_cast<Branch *>(store->get_parent()->get_instrutions()->at(insNum - 2)) != nullptr)
                            {
                                store->insertInstr(store->get_parent(), insNum - 2);
                            }
                            else
                            {
                                store->insertInstr(store->get_parent(), insNum - 1);
                            }
                            for (int j = 0; j < n; j++)
                                if (In[j] == spilledPhis[i])
                                    In[j] = nullptr;
                            d[spilledPhis[i]] = -1;
                            flag = true;
                            break;
                        }
                assert(flag);
            }
        }
    }

    for (auto v : spilledNodes)
    {
        Value *val = LA.Vals[v];
        PHINode *phi = dynamic_cast<PHINode *>(val);
        Instrution *ins = dynamic_cast<Instrution *>(val);
        // Param *para = dynamic_cast<Param *>(val);
        if (phi != nullptr)
        {
            // for (auto it : *(phi->get_valueMap()))
            // {
            //     Store *store = new Store(allocMap[LA.Vals[v]], it.second->get_val(), false, it.first);
            //     int insNum = store->get_parent()->get_instrutions()->size();
            //     if (dynamic_cast<Branch *>(store->get_parent()->get_instrutions()->at(insNum - 2)) != nullptr)
            //     {
            //         store->insertInstr(store->get_parent(), insNum - 2);
            //     }
            //     else
            //     {
            //         store->insertInstr(store->get_parent(), insNum - 1);
            //     }
            // }
        }
        else if (ins != nullptr)
        {
            if (is_a<Assign>(ins) && is_a<GlobalVariable>(ins->get_value_list()->front()->get_val()))
                continue;
            Store *store = new Store(allocMap[LA.Vals[v]], ins, false, ins->get_parent());
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
            if (dynamic_cast<Store *>(use) != nullptr && dynamic_cast<Store *>(use)->get_addr() == allocMap[LA.Vals[v]])
            {
                continue;
            }
            bool flag = true;
            if (dynamic_cast<Call *>(use) != nullptr)
            {
                flag = false;
                if (val->get_type()->get_type() == TypeEnum::F32)
                {
                    int rk = 0;
                    for (auto it : *(use->get_value_list()))
                        if (!is_a<Function>(it->get_val()) && it->get_val()->get_type()->get_type() == TypeEnum::F32)
                        {
                            rk++;
                            if (it->get_val() == val)
                                break;
                        }
                    if (rk <= Para_S)
                        flag = true;
                }
                else if (val->get_type()->get_type() == TypeEnum::VecI32)
                {
                    assert(0);
                }
                else
                {
                    int rk = 0;
                    for (auto it : *(use->get_value_list()))
                        if (!is_a<Function>(it->get_val()) &&
                            it->get_val()->get_type()->get_type() != TypeEnum::F32 &&
                            it->get_val()->get_type()->get_type() != TypeEnum::VecI32 &&
                            (!is_a<Alloca>(it->get_val()) || !isSpill(dynamic_cast<Alloca *>(it->get_val()))))
                        {
                            rk++;
                            if (it->get_val() == val)
                                break;
                        }
                    if (rk <= Para_R)
                        flag = true;
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
                        load = new Load(allocMap[LA.Vals[v]], false, use->get_parent());
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
                    it->set_val(allocMap[LA.Vals[v]]);
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

void SSARegisterAlloc::AddTripleX(Function *p_func)
{
    std::vector<Triple *> Triples_todo;
    for (auto bb : *(p_func->get_blocks()))
        for (auto ins : *(bb->get_instrs()))
        {
            if (ins->get_Instype() == InstrutionEnum::FMLA ||
                ins->get_Instype() == InstrutionEnum::FMLS ||
                ins->get_Instype() == InstrutionEnum::VMLA ||
                ins->get_Instype() == InstrutionEnum::VMLS)
            {
                Triples_todo.push_back(dynamic_cast<Triple *>(ins));
            }
        }
    for (auto ins : Triples_todo)
    {
        assert(ins->get_value_list()->size() == 3);
        Value *val_a = ins->get_operand_at(0);
        Value *val_b = ins->get_operand_at(1);
        Value *val_c = ins->get_operand_at(2);
        TripleX *triplex = new TripleX(InstrutionEnum::TripleX, ins, val_b == val_a ? ins : val_b, val_c == val_a ? ins : val_c, ins->get_parent());
        triplex->insertInstr(ins->get_parent(), std::find(ins->get_parent()->get_instrutions()->begin(), ins->get_parent()->get_instrutions()->end(), ins) - ins->get_parent()->get_instrutions()->begin() + 1);
    }
}

void SSARegisterAlloc::AnalysisTriple(Function *p_func)
{
    for (auto bb : *(p_func->get_blocks()))
        for (auto ins : *(bb->get_instrs()))
        {
            if (ins->get_Instype() == InstrutionEnum::FMLA ||
                ins->get_Instype() == InstrutionEnum::FMLS ||
                ins->get_Instype() == InstrutionEnum::VMLA ||
                ins->get_Instype() == InstrutionEnum::VMLS)
            {
                if (LA.ValueIdMap.find(ins->get_operand_at(0)) == LA.ValueIdMap.end())
                {
                    ins->print();
                    fflush(stdout);
                }
                assert(LA.ValueIdMap.find(ins->get_operand_at(0)) != LA.ValueIdMap.end());
                assert(LA.ValueIdMap.find(ins) != LA.ValueIdMap.end());
                LA.combine(LA.ValueIdMap.at(ins), LA.ValueIdMap.at(ins->get_operand_at(0)));
            }
        }
}

void SSARegisterAlloc::DoTripleX(Function *p_func)
{
    std::vector<TripleX *> removeList;
    for (auto bb : *(p_func->get_blocks()))
    {
        for (auto ins : *(bb->get_instrs()))
        {
            if (is_a<TripleX>(ins))
                removeList.push_back(dynamic_cast<TripleX *>(ins));
        }
    }
    for (auto ins : removeList)
        ins->drop();
}