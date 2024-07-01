#include <ir_opt/ALS.hpp>
#include <ir_opt/DCE.hpp>
#include <ir_opt/SimplifyCFG.hpp>
#include <queue>

DomTree *Vs::domtree = nullptr;

static inline bool _DealBinary(Binary *instr, std::unordered_map<Value *, ALexp *> &map)
{
    ALexp *exp = nullptr;
    Value *src1 = instr->get_src1(), *src2 = instr->get_src2();
    bool flag = true;
    if (is_a<ConstantI32>(src1) && is_a<ConstantI32>(src2))
    {
        int num1 = *((ConstantI32 *)src1)->get_i32().begin();
        int num2 = *((ConstantI32 *)src2)->get_i32().begin();
        exp = new ALexp(num1 + num2);
        flag = false;
    }
    else if (is_a<ConstantI32>(src1))
    {
        auto it = map.find(src2);
        assert(it != map.end());
        int num = *((ConstantI32 *)src1)->get_i32().begin();
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
            exp = new ALexp((*it->second));
            exp->Num += num;
            break;
        case InstrutionEnum::ISUB:
            exp = new ALexp(num);
            *exp = (*exp) - (*it->second);
            break;
        case InstrutionEnum::IMUL:
            exp = new ALexp((*it->second) * num);
            break;
        case InstrutionEnum::IDIV:
        case InstrutionEnum::IMOD:
            exp = new ALexp(instr);
            flag = false;
            break;
        default:
            assert(0);
            break;
        }
    }
    else if (is_a<ConstantI32>(src2))
    {
        auto it = map.find(src1);

        assert(it != map.end());
        int num = *((ConstantI32 *)src2)->get_i32().begin();
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
            exp = new ALexp((*it->second));
            exp->Num += num;
            break;
        case InstrutionEnum::ISUB:
            exp = new ALexp((*it->second));
            exp->Num -= num;
            break;
        case InstrutionEnum::IMUL:
            exp = new ALexp((*it->second) * num);
            break;
        case InstrutionEnum::IDIV:
            if (it->second->checkMod(num))
                exp = new ALexp((*it->second) / num);
            else
            {
                exp = new ALexp(instr);
                flag = false;
            }
            break;
        case InstrutionEnum::IMOD:
            exp = new ALexp(instr);
            flag = false;
            break;
        default:
            assert(0);
            break;
        }
    }
    else
    {
        auto it1 = map.find(src1), it2 = map.find(src2);
        assert(it1 != map.end() && it2 != map.end());
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
            exp = new ALexp((*it1->second) + (*it2->second));
            break;
        case InstrutionEnum::ISUB:
            exp = new ALexp((*it1->second) - (*it2->second));
            break;
        case InstrutionEnum::IMUL:
        case InstrutionEnum::IDIV:
        case InstrutionEnum::IMOD:
            exp = new ALexp(instr);
            flag = false;
            break;
        default:
            assert(0);
            break;
        }
    }
    assert(exp != nullptr);
    map.insert({instr, exp});
    if (!flag)
        return false;
    auto _List = src1->get_user_list();
    std::set<Value *> vis;
    for (auto it : *_List)
        vis.insert(it->get_user());
    if (vis.size() == 1)
        return true;
    vis.clear();
    _List = src2->get_user_list();
    for (auto it : *_List)
        vis.insert(it->get_user());
    if (vis.size() == 1)
        return true;
    else
        return false;
}

static inline bool _DealUnary(Unary *instr, std::unordered_map<Value *, ALexp *> &map)
{
    Value *src = instr->get_src();
    if (is_a<Constant>(src))
        return false;
    ALexp *exp = new ALexp(0);
    auto it = map.find(src);
    assert(it != map.end());
    *exp = *exp - (*it->second);
    map.insert({instr, exp});
    auto _List = src->get_user_list();
    std::set<Value *> vis;
    for (auto it : *_List)
        vis.insert(it->get_user());
    if (vis.size() == 1)
        return true;
    else
        return false;
}

void ALS::FuncDealer(Function *func)
{
    std::set<Value *> vis;
    std::queue<BasicBlock *> q1, q2;
    BasicBlock *addBB = new BasicBlock(func);
    q1.push(func->get_entryBB());
    vis.insert(func->get_entryBB());
    auto params = func->get_params();

    for (Param *Param : *params)
    {
        ALexp *exp = new ALexp(Param);
        map.insert({Param, exp});
    }
    std::vector<Value *> Del;
    while (!q1.empty())
    {
        std::swap(q1, q2);
        while (!q2.empty())
        {
            BasicBlock *curBB = q2.front();
            q2.pop();
            for (auto it : *curBB->get_phinodes())
            {
                ALexp *exp = new ALexp(it);
                map.insert({it, exp});
            }
            for (auto it : *curBB->get_user_list())
            {
                Value *user = it->get_user();
                if (vis.find(user) != vis.end())
                    continue;
                vis.insert(user);
                q1.push((BasicBlock *)user);
            }

            auto instrs = curBB->get_instrs();
            bool flag = true;
            for (auto &instr : *instrs)
            {
                if (instr->get_type()->get_type() == TypeEnum::F32)
                    continue;
                if (instr->isBinary())
                    flag = _DealBinary((Binary *)instr, map);
                else if (instr->get_Instrtype() == InstrutionEnum::MINUS)
                    flag = _DealUnary((Unary *)instr, map);
                else
                {
                    ALexp *exp = new ALexp(instr);
                    map.insert({instr, exp});
                    continue;
                }
                auto it = map.find(instr);
                assert(it != map.end());
                ALexp *exp = it->second;
                if (!flag)
                    continue;
                if (exp->exps.size() == 0)
                {
                    Constant *constnum = new ConstantI32(exp->Num);
                    func->value_pushBack(constnum);
                    auto _list = instr->get_user_list();
                    for (auto &it : *_list)
                        it->set_val(constnum);
                    _list->clear();
                }
                else if (exp->exps.size() == 1 && exp->Num == 0 && (((*exp->exps.begin()).second) >= 6 || (*exp->exps.begin()).second == 1))
                {
                    auto srcs = *exp->exps.begin();
                    Constant *constnum = new ConstantI32(srcs.second);
                    func->value_pushBack(constnum);
                    Instrution *p_instr = new Binary(InstrutionEnum::IMUL, srcs.first, constnum, addBB);
                    addBB->Ins_popBack();
                    auto _list = instr->get_user_list();
                    for (auto &it : *_list)
                        it->set_val(p_instr);
                    _list->clear();
                    Del.emplace_back(instr);
                    instr = p_instr;
                    p_instr->setParent_F(curBB);
                    _DealBinary((Binary *)instr, map);
                }
            }
        }
    }
    for (auto it : Del)
        it->drop();
    auto _list = func->get_blocks();
    for (auto it = _list->begin(); it != _list->end(); ++it)
        if (*it == addBB)
        {
            _list->erase(it);
            break;
        }
    delete addBB;
    for (auto &it : map)
        delete it.second;
    map.clear();
}

void ALS::FuncCSS(Function *func)
{
    std::set<Value *> vis;
    std::queue<BasicBlock *> q1, q2;
    std::unordered_map<ALexp, Vs *, ALexpHash> ALexpMap;
    q1.push(func->get_entryBB());
    vis.insert(func->get_entryBB());
    auto params = func->get_params();
    for (Param *Param : *params)
    {
        ALexp *exp = new ALexp(Param);
        map.emplace(Param, exp);
        Vs *newVs = new Vs();
        newVs->vals.emplace_back(Param);
        ALexpMap.emplace(*exp, newVs);
    }
    std::vector<Value *> Del;
    while (!q1.empty())
    {
        std::swap(q1, q2);
        while (!q2.empty())
        {
            BasicBlock *curBB = q2.front();
            q2.pop();
            for (auto it : *curBB->get_phinodes())
            {
                ALexp *exp = new ALexp(it);
                map.emplace(it, exp);
                Vs *newVs = new Vs();
                newVs->vals.emplace_back(it);
                ALexpMap.emplace(*exp, newVs);
            }
            for (auto it : *curBB->get_user_list())
            {
                Value *user = it->get_user();
                if (vis.find(user) != vis.end())
                    continue;
                vis.insert(user);
                q1.push((BasicBlock *)user);
            }

            auto instrs = curBB->get_instrs();
            for (auto &instr : *instrs)
            {
                if (instr->get_type()->get_type() == TypeEnum::F32)
                    continue;
                if (instr->isBinary())
                    _DealBinary((Binary *)instr, map);
                else if (instr->get_Instrtype() == InstrutionEnum::MINUS)
                    _DealUnary((Unary *)instr, map);
                else
                {
                    ALexp *exp = new ALexp(instr);
                    map.insert({instr, exp});
                    continue;
                }
                auto it = map.find(instr);
                assert(it != map.end());
                ALexp *exp = it->second;
                auto ALfind = ALexpMap.find(*exp);
                if (ALfind == ALexpMap.end())
                {
                    Vs *newVs = new Vs();
                    newVs->vals.emplace_back(instr);
                    ALexpMap.emplace(*exp, newVs);
                }
                else if (!ALfind->second->replaceVal(instr))
                    ALfind->second->vals.emplace_back(instr);
            }
        }
    }
    for (auto &it : map)
        delete it.second;
    for (auto &it : ALexpMap)
        delete it.second;

    map.clear();
}

static inline void DIVCombine(Function *func)
{
    std::set<Value *> vis;
    std::queue<BasicBlock *> q1, q2;
    q1.push(func->get_entryBB());
    vis.insert(func->get_entryBB());
    std::vector<Value *> Del;
    while (!q1.empty())
    {
        std::swap(q1, q2);
        while (!q2.empty())
        {
            BasicBlock *curBB = q2.front();
            q2.pop();

            for (auto it : *curBB->get_user_list())
            {
                Value *user = it->get_user();
                if (vis.find(user) != vis.end())
                    continue;
                vis.insert(user);
                q1.push((BasicBlock *)user);
            }

            auto instrs = curBB->get_instrs();
            for (Instrution *instr : *instrs)
            {
                if (instr->get_Instrtype() != InstrutionEnum::IDIV)
                    continue;
                Binary *p_b = (Binary *)instr;
                if (!is_a<Instrution>(p_b->get_src1()))
                    continue;
                Instrution *src1 = (Instrution *)p_b->get_src1();
                if (src1->get_Instrtype() != InstrutionEnum::IMUL)
                    continue;
                Binary *p_mul = (Binary *)src1;
                Value *p_replace = nullptr;
                if (p_mul->get_src1() == p_b->get_src2())
                    p_replace = p_mul->get_src2();
                else if (p_mul->get_src2() == p_b->get_src2())
                    p_replace = p_mul->get_src1();
                if (p_replace == nullptr)
                    continue;
                // assert(0);
                auto _list = instr->get_user_list();
                for (auto edge : *_list)
                    edge->set_val(p_replace);
                _list->clear();
            }
        }
    }
}

void ALS::PassRun(Module *_module)
{
    puts("            ALS  BEGIN               ");
    p_module = _module;
    Vs::domtree = nullptr;
    for (Function *p_func : *_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
        DIVCombine(p_func);
        DomTree tree(p_func);
        tree.Run();
        Vs::domtree = &tree;
        FuncCSS(p_func);
        Vs::domtree = nullptr;
    }
    puts("            ALS  END              ");
}
