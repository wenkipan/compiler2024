#include <ir_opt/MAD.hpp>
#include <ir_opt/DCE.hpp>

void MAD::LDD()
{
    std::unordered_map<Value *, Value *> ValMap;
    for (BasicBlock *BB : *func->get_blocks())
    {
        auto instrs = BB->get_instrs();
        ValMap.clear();
        for (Instrution *instr : *instrs)
        {
            if (instr->isStore())
            {
                Store *ST = (Store *)instr;
                auto it = ValMap.find(ST->get_addr());
                if (it == ValMap.end())
                    ValMap.emplace(ST->get_addr(), ST->get_src());
                else
                    it->second = ST->get_src();
            }
            else if (instr->isLoad())
            {
                Load *LD = (Load *)instr;
                auto it = ValMap.find(LD->get_addr());
                if (it != ValMap.end())
                {
                    Value *src = it->second;
                    auto _list = LD->get_user_list();
                    for (Edge *edge : *_list)
                        edge->set_val(src);
                    _list->clear();
                }
            }
            else if (instr->isCall())
            {
                Call *p_call = (Call *)instr;
                if (!((Function *)p_call->get_func())->get_isExternal())
                    ValMap.clear();
            }
        }
    }
    DCE woker;
    woker.run(func);
}

void MAD::SSD()
{
    for (BasicBlock *BB : *func->get_blocks())
    {
        std::vector<Instrution *> Del;
        auto instrs = BB->get_instrs();
        for (auto it = instrs->begin(); it != instrs->end(); ++it)
        {
            if (!(*it)->isStore())
                continue;
            Store *ST1 = (Store *)*it;
            for (auto jt = it + 1; jt != instrs->end(); ++jt)
            {
                if ((*jt)->isCall())
                    break;
                else if ((*jt)->isStore())
                {
                    Store *ST2 = (Store *)*jt;
                    if (ST1->get_addr() == ST2->get_addr())
                    {
                        Del.emplace_back(ST1);
                        break;
                    }
                }
            }
        }
        for (Instrution *instr : Del)
            instr->drop();
    }
    DCE woker;
    woker.run(func);
}

static bool _dfs(GEP *gep, std::set<GEP *> &geps)
{
    if (!is_a<ConstantI32>(gep->get_offset()))
        return false;
    geps.insert(gep);
    bool flag = true;
    auto _list = gep->get_user_list();
    for (Edge *edge : *_list)
    {
        Instrution *instr = (Instrution *)edge->get_user();
        if (instr->isCall())
            return false;
        else if (instr->isGEP())
            flag = flag & _dfs((GEP *)instr, geps);
        else if (instr->isLoad() || instr->isStore())
            continue;
        else
        {
            instr->print();
            assert(0);
        }
    }
    return flag;
}

static inline Store *_IF_1ST(GEP *gep, Function *func)
{
    int cnt = 0;
    Store *ST = nullptr;
    auto _list = gep->get_user_list();
    for (Edge *edge : *_list)
    {
        Instrution *user = (Instrution *)edge->get_user();
        if (!user->isStore())
            continue;
        ++cnt;
        ST = (Store *)user;
    }
    if (cnt != 1 || ST->get_parent() != func->get_entryBB())
        return nullptr;
    for (Edge *edge : *_list)
    {
        Instrution *user = (Instrution *)edge->get_user();
        if (user->isStore() || user->get_parent() != func->get_entryBB())
            continue;
        auto instrs = func->get_entryBB()->get_instrs();
        for (Instrution *instr : *instrs)
        {
            if (instr == user)
                return nullptr;
            else if (instr == ST)
                break;
        }
    }
    return ST;
}

void MAD::UAD()
{
    for (BasicBlock *BB : *func->get_blocks())
    {
        auto instrs = BB->get_instrs();
        for (Instrution *instr : *instrs)
        {
            if (instr->isCall())
            {
                Call *p_call = (Call *)instr;
                Function *func = (Function *)p_call->get_func();
                if (!func->get_isExternal())
                    return;
            }
            else if (instr->isAlloca())
            {
                if (BB != func->get_entryBB())
                {
                    BB->print();
                }
                assert(BB == func->get_entryBB());
                assert(instr->get_type()->get_type() == TypeEnum::Ptr);
                Ptr *Tptr = (Ptr *)instr->get_type();
                if (Tptr->get_btype()->get_type() == TypeEnum::Array)
                {
                    auto _list = instr->get_user_list();
                    bool flag = true;
                    std::set<GEP *> geps;
                    for (Edge *edge : *_list)
                    {
                        Instrution *user = (Instrution *)edge->get_user();
                        if (user->isCall())
                            flag = false;
                        else if (user->isGEP())
                            flag = flag & _dfs((GEP *)user, geps);
                        if (!flag)
                            break;
                    }
                    if (!flag)
                        continue;
                    for (GEP *gep : geps)
                    {
                        Store *ST = nullptr;
                        if ((ST = _IF_1ST(gep, func)) != nullptr)
                        {
                            auto users = gep->get_user_list();
                            for (Edge *edge : *users)
                            {
                                Instrution *user = (Instrution *)edge->get_user();
                                if (user->isLoad())
                                    user->replaceAllUses(ST->get_src());
                            }
                        }
                    }
                }
            }
        }
    }
    DCE woker;
    woker.run(func);
}

bool MAD::_findST(GEP *gep, Function *func, int cnt, Call *&_call)
{
    nwDels.emplace_back(gep);
    bool flag = true;
    auto _list = gep->get_user_list();

    for (Edge *edge : *_list)
    {
        assert(is_a<Instrution>(edge->get_user()));
        Instrution *user = (Instrution *)edge->get_user();
        if (user->isCall())
        {
            Call *p_call = (Call *)user;
            if (((Function *)p_call->get_func())->get_name() == "memset" && p_call->get_parent() == func->get_entryBB())
            {
                if (_call == nullptr && cnt == 1)
                {
                    _call = p_call;
                    continue;
                }
                else
                    flag = false;
            }
            flag = false;
            break;
        }
        else if (user->isGEP())
        {
            flag = flag & _findST((GEP *)user, func, cnt + 1, _call);
        }
        else if (user->isStore())
            nwDels.emplace_back(user);
        else if (user->isLoad())
            return false;
    }
    return flag;
}

void MAD::ARD()
{
    std::set<Instrution *> Del;
    auto instrs = func->get_entryBB()->get_instrs();
    for (auto instr : *instrs)
    {
        if (!instr->isAlloca())
            continue;
        bool flag = true;
        nwDels.clear();
        Call *p_call = nullptr;
        auto _list = instr->get_user_list();
        for (Edge *edge : *_list)
        {
            Instrution *user = (Instrution *)edge->get_user();
            if (user->isCall())
            {
                assert(0);
            }
            else if (user->isGEP())
                flag = flag & _findST((GEP *)user, func, 1, p_call);
            else if (user->isLoad())
                flag = false;
            if (!flag)
                break;
        }
        if (!flag)
            continue;
        Del.insert(instr);
        if (p_call)
            Del.insert(p_call);
        for (Instrution *_del : nwDels)
            Del.insert(_del);
    }
    for (Instrution *_del : Del)
        _del->drop();
    DCE woker;
    woker.run(func);
}

static int _getMAs(GEP *gep, std::set<Store *> &STs, std::set<Load *> &LDs, std::set<GEP *> &GEPs, Call *&call, bool FIT)
{
    auto user = gep->get_user_list();
    int flag = 0;
    for (Edge *edge : *user)
    {
        int tmp = 0;
        Instrution *user = (Instrution *)edge->get_user();
        if (user->isGEP())
            tmp = _getMAs((GEP *)user, STs, LDs, GEPs, call, false);
        else if (user->isLoad())
            LDs.insert((Load *)user);
        else if (user->isStore())
        {
            Store *ST = (Store *)user;
            if (!is_a<ConstantI32>(ST->get_src()))
                return -1;
            STs.insert(ST);
            tmp = 1;
        }
        else if (user->isCall())
        {
            if (!FIT || call != nullptr)
                return -1;
            Call *p_call = (Call *)user;
            if (((Function *)p_call->get_func())->get_name() == "memset")
            {
                if (is_a<ConstantI32>(gep->get_offset()))
                {
                    if ((*((ConstantI32 *)gep->get_offset())->get_i32().begin()) == 0)
                        call = p_call;
                    else
                        return -1;
                }
                else
                    return -1;
            }
            else
                return -1;
        }
        if (tmp == -1)
            return -1;
        else if (tmp == 1)
            flag = 1;
    }
    if (flag)
    {
        if (is_a<ConstantI32>(gep->get_offset()))
        {
            GEPs.insert(gep);
            return 1;
        }
        else
            return -1;
    }
    return 0;
}

void MAD::SMO()
{
    std::set<Store *> STs;
    std::set<Load *> LDs;
    std::set<GEP *> GEPs;
    DomTree tree(func);
    tree.Run();
    std::set<Value *> Allocas;
    auto instrs = func->get_entryBB()->get_instrs();
    for (auto instr = instrs->begin(); instr != instrs->end(); ++instr)
    {
        if (!(*instr)->isAlloca())
            continue;
        if ((((Ptr *)(*instr)->get_type())->get_btype()->get_type() != TypeEnum::Array))
            assert(0);
        if (Allocas.find(*instr) != Allocas.end())
            continue;
        STs.clear(), LDs.clear(), GEPs.clear();
        auto users = (*instr)->get_user_list();
        int flag;
        Call *call = nullptr;
        for (Edge *edge : *users)
        {
            Instrution *user = (Instrution *)edge->get_user();
            assert(user->isGEP());
            flag = _getMAs((GEP *)user, STs, LDs, GEPs, call, true);
            if (flag == -1)
                break;
        }
        if (flag == -1)
            continue;
        std::set<Value *> vis;
        BasicBlock *CallBB = nullptr;
        if (call != nullptr)
            CallBB = call->get_parent();
        for (Store *ST : STs)
        {
            BasicBlock *STBB = ST->get_parent();
            if (vis.find(ST->get_src()) != vis.end())
            {
                flag = -1;
                break;
            }
            vis.insert(ST->get_src());
            if (call != nullptr)
            {
                if (STBB == CallBB)
                {
                    auto instrs = STBB->get_instrs();
                    for (Instrution *instr : *instrs)
                        if (instr == ST)
                        {
                            flag = -1;
                            break;
                        }
                        else if (instr == call)
                            break;
                }
                else if (tree.is_dom(CallBB, STBB))
                    ;
                else
                    flag = -1;
            }

            for (Load *LD : LDs)
            {
                BasicBlock *LDBB = LD->get_parent();
                if (STBB == LDBB)
                {
                    auto instrs = STBB->get_instrs();
                    for (Instrution *instr : *instrs)
                        if (instr == LD)
                        {
                            flag = -1;
                            break;
                        }
                        else if (instr == ST)
                            break;
                }
                else if (tree.is_dom(STBB, LDBB))
                    ;
                else
                    flag = -1;
                if (flag == -1)
                    break;
            }
            if (flag == -1)
                break;
        }
        if (flag == -1)
            continue;
        Allocas.insert(*instr);
        BasicBlock *BB = func->get_entryBB();
        Branch *p_branch = (Branch *)BB->get_last_instrution();
        BB->Ins_popBack();
        Instrution *cond = nullptr;
        if (p_branch->isBranch())
        {
            assert(p_branch->get_cond() == BB->get_last_instrution());
            cond = BB->get_last_instrution();
            BB->Ins_popBack();
        }
        for (GEP *gep : GEPs)
            if (gep->get_parent() != BB)
                gep->insertInstr(BB, BB->get_instrs()->size());
        for (Store *ST : STs)
            if (ST->get_parent() != BB)
                ST->insertInstr(BB, BB->get_instrs()->size());
        if (cond != nullptr)
            BB->Ins_pushBack(cond);
        BB->Ins_pushBack(p_branch);
        instr = instrs->begin();
    }
}

void MAD::FuncDealer()
{
    LDD(); // stroe a A; b = load A -> b = a;
    SSD(); // store a A; no load A ; store b A -> store b A
    UAD(); // only store del
    ARD(); // no load array del

    SMO(); // store dom all load move to top
}

void MAD::PassRun(Module *p_module)
{
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        func = p_func;
        FuncDealer();
    }
}