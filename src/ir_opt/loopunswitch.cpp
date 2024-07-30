#include <unordered_map>
#include <iostream>
#include <algorithm>

#include "../../include/ir_opt/loopunswitch.hpp"

#define MustDo(it, list_) ((it) == (list_)->end() ? Assert_() : true)

static inline void ResetVal_(PHINode *phi, Value *val, BasicBlock *BB, IRCopy &copyer)
{
    PHINode *cphi = (PHINode *)copyer.get_mapval(phi);
    BasicBlock *cBB = copyer.get_mapbb(BB);
    cphi->eraseIncoming(cBB);
    cphi->addIncoming(val, cBB);
}

void loopunswitch::SetVal(PHINode *phi, IRCopy &copyer, Loop *loop)
{
    auto map_ = phi->get_valueMap();
    for (auto it_ : *map_)
    {
        assert(loop->is_BBinLoop(it_.first) || loop->get_prev() == it_.first);
        Value *val = it_.second->get_val();
        if (is_a<Param>(val) || is_a<Constant>(val))
            ResetVal_(phi, val, it_.first, copyer);
        else if (is_a<Instrution>(val))
        {
            Instrution *instr_ = (Instrution *)val;
            BasicBlock *BB_ = instr_->get_parent();
            if (!loop->is_BBinLoop(BB_))
                ResetVal_(phi, val, it_.first, copyer);
        }
        else
            assert(0);
    }
}

void loopunswitch::SetUser(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list_ = instr->get_user_list();
    for (auto it_ : *list_)
    {
        Instrution *T = (Instrution *)it_->get_user();
        if (loop->is_BBinLoop(T->get_parent()))
            continue;
        assert(is_a<PHINode>(it_->get_user()));
        PHINode *user = (PHINode *)it_->get_user();
        BasicBlock *BB = user->get_edge_income_block(it_);
        assert(loop->is_BBinLoop(BB));
        PHINode *cuser = (PHINode *)copyer.get_mapval(user);
        BasicBlock *cBB = copyer.get_mapbb(BB);
        cuser->eraseIncoming(cBB);
        user->addIncoming(copyer.get_mapval(instr), cBB);
        puts("???");
    }
}

static inline bool Assert_()
{
    assert(0);
    return false;
}

static inline void ResetVal_(Instrution *instr, Value *val, IRCopy &copyer)
{
    Instrution *cinstr = (Instrution *)copyer.get_mapval(instr);
    Value *cval = copyer.get_mapval(val);
    auto list_ = cval->get_user_list();
    for (auto it = list_->begin(); MustDo(it, list_); ++it)
        if ((*it)->get_user() == cinstr)
        {
            (*it)->set_val(val);
            list_->erase(it);
            break;
        }
}

void loopunswitch::SetVal(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list_ = instr->get_value_list();
    for (auto it : *list_)
    {
        Value *val = it->get_val();
        if (is_a<Param>(val) || is_a<Constant>(val))
            ResetVal_(instr, val, copyer);
        else if (is_a<Instrution>(val))
        {
            Instrution *instr_ = (Instrution *)val;
            BasicBlock *BB_ = instr_->get_parent();
            if (!loop->is_BBinLoop(BB_))
                ResetVal_(instr, val, copyer);
        }
        else if (is_a<GlobalValue>(val))
            continue;
        else
            assert(0);
    }
}

static inline bool SSSSCheck(Value *val, Function *func) // need delete in release
{
    auto list = func->get_Values();
    for (Value *it : *list)
        if (it == val)
            return true;
    return false;
}

void loopunswitch::SetPrevBB(BasicBlock *workBB, Loop *loop, IRCopy &copyer)
{
    BasicBlock *prevBB = loop->get_prev();
    BasicBlock *cprevBB = copyer.get_mapbb(prevBB);
    BasicBlock *Header = loop->get_header();
    BasicBlock *cHeader = copyer.get_mapbb(Header);
    for (auto it : *cHeader->get_phinodes())
    {
        Value *val = it->get_valueMap()->find(cprevBB)->second->get_val();
        SSSSCheck(val, workBB->get_func());
        it->eraseIncoming(cprevBB);
        it->addIncoming(val, prevBB);
    }

    assert(prevBB->get_last_instrution()->isJmp());
    auto list_ = Header->get_value_list();
    auto it = list_->begin();
    for (; MustDo(it, list_); ++it)
        if ((*it)->get_val() == prevBB)
        {
            delete *it;
            list_->erase(it);
            break;
        }
    prevBB->get_user_list()->clear();
    list_ = cHeader->get_value_list();
    it = list_->begin();
    for (; MustDo(it, list_); ++it)
        if ((*it)->get_val() == cprevBB)
        {
            delete *it;
            list_->erase(it);
            break;
        }
    cprevBB->get_user_list()->clear();
    int cnt = prevBB->get_instrs()->size();
    prevBB->get_last_instrution()->drop();
    assert(cnt == prevBB->get_instrs()->size() + 1);
    Branch *Wbranch = (Branch *)workBB->get_last_instrution();
    workBB->Ins_popBack();
    assert(workBB->get_last_instrution()->isCmp());
    Cmp *Wcmp = (Cmp *)workBB->get_last_instrution();
    workBB->Ins_popBack();
    Cmp *prevCmp = new Cmp(Wcmp->get_Instrtype(), Wcmp->get_src1(), Wcmp->get_src2(), prevBB);
    new Branch(prevCmp, Header, cHeader, prevBB);
    BasicBlock *cworkBB = copyer.get_mapbb(workBB);
    Branch *cbranch = (Branch *)(cworkBB)->get_last_instrution();
    cworkBB->Ins_popBack();
    Cmp *cWcmp = (Cmp *)copyer.get_mapval(Wcmp);
    assert(cWcmp->get_user_list()->size() == 1 && cWcmp == cworkBB->get_last_instrution());
    Constant *num0 = new ConstantI32(0);
    workBB->get_func()->value_pushBack(num0);
    Cmp *NcWcmp = new Cmp(InstrutionEnum::INEQ, num0, num0, cworkBB);
    (*cWcmp->get_user_list()->begin())->set_val(NcWcmp);
    cWcmp->get_user_list()->clear();
    cworkBB->Ins_pushBack(cbranch);
    Cmp *NWcmp = new Cmp(InstrutionEnum::IEQ, num0, num0, workBB);
    (*Wcmp->get_user_list()->begin())->set_val(NWcmp);
    Wcmp->get_user_list()->clear();
    Wcmp->drop(), cWcmp->drop();
    workBB->Ins_pushBack(Wbranch);
}

void loopunswitch::Unswitch(Loop *loop, Function *p_func, BasicBlock *workBB, DomTree &domtree)
{
    IRCopy copyer;
    Function *cfunc = copyer.copy_func(p_func);
    std::vector<Value *> addvals;
    std::vector<BasicBlock *> addBBs;
    for (BasicBlock *BB : *loop->get_BBs())
    {
        addBBs.emplace_back(copyer.get_mapbb(BB));
        for (PHINode *phi : *BB->get_phinodes())
        {
            addvals.emplace_back(copyer.get_mapval(phi));
            SetVal(phi, copyer, loop);
            SetUser(phi, copyer, loop);
        }
        for (Instrution *instr : *BB->get_instrutions())
        {
            addvals.emplace_back(copyer.get_mapval(instr));
            SetVal(instr, copyer, loop);
            SetUser(instr, copyer, loop);
        }
    }
    for (auto it : addBBs)
    {
        auto list_ = cfunc->get_blocks();
        for (auto del = list_->begin(); MustDo(del, list_); ++del)
            if ((*del) == it)
            {
                it->Set_parent(p_func);
                p_func->get_blocks()->emplace_back(it);
                list_->erase(del);
                break;
            }
    }
    for (auto it : addvals)
    {
        auto list_ = cfunc->get_Values();
        for (auto del = list_->begin(); MustDo(del, list_); ++del)
            if ((*del) == it)
            {
                p_func->get_Values()->emplace_back(it);
                list_->erase(del);
                break;
            }
    }
    SetPrevBB(workBB, loop, copyer);
    for (BasicBlock *BB : *loop->get_BBs())
    {
        auto list = BB->get_user_list();
        for (auto edge : *list)
        {
            BasicBlock *nextBB = (BasicBlock *)edge->get_user();
            if (loop->is_BBinLoop(nextBB))
                continue;
            BasicBlock *cnextBB = copyer.get_mapbb(nextBB);
            auto users = copyer.get_mapbb(BB)->get_user_list();
            for (auto cedge : *users)
            {
                if (cedge->get_user() == cnextBB)
                {
                    cedge->set_user(nextBB);
                }
            }
        }
    }
    auto list = cfunc->get_Values();
    for (Value *val : *list)
    {
        auto edges = val->get_value_list();
        for (auto edge : *edges)
        {
            Value *value = edge->get_val();
            if (is_a<GlobalValue>(value))
            {
                auto users = value->get_user_list();
                for (auto it = users->begin(); it != users->end(); ++it)
                {
                    if ((*it) == edge)
                    {
                        delete edge;
                        users->erase(it);
                        break;
                    }
                }
            }
        }
    }

    delete cfunc;
}

static inline bool _CheckVar(Value *val, Loop *loop)
{
    if (is_a<Constant>(val) || is_a<Param>(val))
        return true;
    if (loop->is_BBinLoop(((Instrution *)val)->get_parent()))
        return false;
    else
        return true;
    assert(0);
    return false;
}

BasicBlock *loopunswitch::checkIF(Loop *loop)
{
    BasicBlock *re = nullptr;
    for (BasicBlock *BB : *loop->get_nwBBs())
    {
        Instrution *p_branch = BB->get_last_instrution();
        if (!p_branch->isBranch())
            continue;
        Cmp *p_cond = (Cmp *)((Branch *)p_branch)->get_cond();
        if (_CheckVar(p_cond->get_src1(), loop) && _CheckVar(p_cond->get_src2(), loop))
            re = BB;
    }
    int cnt = 0;
    for (BasicBlock *BB : *loop->get_nwBBs())
    {
        cnt += BB->get_instrs()->size();
        cnt += BB->get_phinodes()->size();
    }
    if (cnt >= 1000)
        re = nullptr;
    else if (cnt >= 500)
    {
        if (codesize > 3000)
            re = nullptr;
        else
            codesize += cnt;
    }
    else if (cnt >= 150)
    {
        if (codesize > 4500)
            re = nullptr;
        else
            codesize += cnt;
    }
    else if (codesize > 7000)
        re = nullptr;
    else
        codesize += cnt;
    return re;
}

void loopunswitch::searchFunc(Loop *loop, Function *func, DomTree &domtree, bool &flag)
{
    for (Loop *son : *loop->get_lpsons())
        searchFunc(son, func, domtree, flag);
    if (flag)
        return;
    BasicBlock *BB = checkIF(loop);
    if (BB == nullptr)
        return;
    flag = true;
    Unswitch(loop, func, BB, domtree);
}

static inline void _checkVal(Value *val, Loop *loop)
{

    auto userList = val->get_user_list();
    auto exitslist = loop->get_exits();
    for (auto it = userList->begin(); it != userList->end(); ++it)
    {
        Instrution *_user = (Instrution *)(*it)->get_user();
        BasicBlock *_BB = _user->get_parent();
        if (loop->is_BBinLoop(_BB))
            continue;
        if (_user->get_Instrtype() == InstrutionEnum::PHINode && exitslist->find(_BB) != exitslist->end())
        {
            continue;
        }

        puts("error");
        std::cout << exitslist->size() << std::endl;
        (*exitslist->begin())->print();
        _BB->print();
        val->print();
        puts("");
        _user->print();
        assert(0);
    }
    // assert(cnt <= N);
}

static void _checkloop(Loop *loop)
{
    for (Loop *son : *loop->get_lpsons())
        _checkloop(son);
    if (loop->get_header() == nullptr)
    {
        return;
    }
    for (BasicBlock *_BB : *loop->get_nwBBs())
    {

        for (PHINode *_Phi : *_BB->get_phinodes())
            _checkVal(_Phi, loop);
        std::vector<Instrution *> *instrs = _BB->get_instrutions();
        for (Instrution *p_instr : *instrs)
            _checkVal(p_instr, loop);
    }
}

void loopunswitch::FuncDealer(Function *p_func)
{

    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    std::cout << p_func->get_name() << std::endl;
    LCSSA worker(p_func, _Loop);
    worker.run();
    _checkloop(loop);
    DomTree domtree(p_func);
    domtree.Run();
    bool flag = false;
    for (Loop *son : *loop->get_lpsons())
    {
        if (flag)
            break;
        searchFunc(son, p_func, domtree, flag);
    }
}

void loopunswitch::PassRun(Module *p_module)
{
    codesize = 0;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        codesize += p_func->get_Values()->size();
    }

    puts("\n Unswitch\n");
    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}

loopunswitch::loopunswitch()
{
    _SCEV = new SCEV();
}
loopunswitch::~loopunswitch()
{
    delete _SCEV;
}
