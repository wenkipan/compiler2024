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
}

void MAD::FuncDealer()
{

    LDD();
    SSD();
    UAD();
    ARD();
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