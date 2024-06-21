#include <ir_opt/loopunswitch.hpp>
#include <ir_opt/IRCopy.hpp>
#include <unordered_map>
#include <iostream>
#include <algorithm>

static inline void _SetNewVal(Value *_val, Value *_used, IRCopy &copyer)
{
    // Edge *_newEdge = new Edge(_val, _used);
    //_val->get_value_list()->pop_back();
    Value *_key = copyer.get_mapval(_used);
    bool flag = false;
    for (auto i = _val->get_value_list()->begin(); i != _val->get_value_list()->end(); ++i)
        if ((*i)->get_val() == _key)
        {
            for (auto j = _key->get_user_list()->begin(); j != _key->get_user_list()->end(); ++j)
            {
                if ((*j) == (*i))
                {
                    _key->get_user_list()->erase(j);
                    flag = true;
                    break;
                }
            }
            assert(flag);
            (*i)->set_val(_used);
            return;
        }
    assert(0);
}

static inline void _PhiSetNewVal(PHINode *oldPhi, BasicBlock *fromBB, Value *used, IRCopy &copyer)
{
    PHINode *nwPhi = (PHINode *)copyer.get_mapval(oldPhi);
    BasicBlock *nwfromBB = copyer.get_mapbb(fromBB);
    nwPhi->eraseIncoming(nwfromBB);
    nwPhi->addIncoming(used, fromBB);
}

static inline void _dealVal(Value *_val, Loop *loop, std::set<Value *> &_addValues, IRCopy &copyer)
{
    bool isPhi = false;
    if (((Instrution *)_val)->isPHINode())
        isPhi = true;
    std::vector<Edge *> *Edges = _val->get_value_list();
    for (auto it : *Edges)
    {
        Value *_used = (*it).get_val();
        if (is_a<Constant>(_used))
            _addValues.insert(copyer.get_mapval(_used));
        else if (is_a<Param>(_used))
        {
            assert(!isPhi);
            _SetNewVal(copyer.get_mapval(_val), _used, copyer);
        }
        else if (is_a<GlobalValue>(_used))
            continue;
        else if (!loop->is_BBinLoop((((Instrution *)_used)->get_parent())))
        {
            if (isPhi)
            {
                PHINode *oldPhi = (PHINode *)_val;
                _PhiSetNewVal(oldPhi, oldPhi->get_edge_income_block(it), _used, copyer);
            }
            else
            {
                _SetNewVal(copyer.get_mapval(_val), _used, copyer);
            }
        }
    }
}

static inline void _dealUser(Value *val, Loop *loop, IRCopy &copyer, DomTree &domtree)
{
    auto userlist = val->get_user_list();
    for (auto it = userlist->begin(); it != userlist->end(); ++it)
    {
        Instrution *_user = (Instrution *)(*it)->get_user();
        BasicBlock *_BB = _user->get_parent();
        if (loop->is_BBinLoop(_BB))
        {
            continue;
        }
        PHINode *nwPhi = (PHINode *)_user;
        assert(loop->get_exits()->find(_BB) != loop->get_exits()->end());
        Value *Cval = copyer.get_mapval(val);
        Value *CPhi = copyer.get_mapval(nwPhi);
        auto Cuserlist = Cval->get_user_list();

        bool Dflag = false;
        for (auto Edge = Cuserlist->begin(); Edge != Cuserlist->end(); ++Edge)
        {
            if ((*Edge)->get_user() == CPhi)
            {
                delete *Edge;
                Cuserlist->erase(Edge);
                nwPhi->addIncoming(Cval, copyer.get_mapbb(nwPhi->get_edge_income_block(*it)));
                Dflag = true;
                break;
            }
        }
        assert(Dflag);
    }
}

static inline void _Dealexits(Loop *loop, IRCopy &copyer)
{
    for (BasicBlock *BB : *loop->get_BBs())
    {
        Instrution *p_instr = BB->get_last_instrution();
        if (!p_instr->isBranch())
            continue;
        auto userlist = BB->get_user_list();
        assert(userlist->size() == 2);
        int cnt = 0;
        for (auto it = userlist->begin(); it != userlist->end(); ++it, ++cnt)
        {
            BasicBlock *toBB = (BasicBlock *)(*it)->get_user();
            if (loop->is_BBinLoop(toBB))
                continue;
            BasicBlock *CBB = copyer.get_mapbb(BB);
            (*CBB->get_user_list())[cnt]->set_user(toBB);
            int cnt = toBB->get_value_list()->size();
            for (PHINode *Phi : *toBB->get_phinodes())
            {
                if (Phi->get_valueMap()->size() == cnt)
                    continue;
                Phi->addIncoming(Phi->get_valueMap()->find(BB)->second->get_val(), CBB);
                assert(0);
                assert(Phi->get_valueMap()->size() == cnt);
            }
        }
    }
}

static inline Cmp *_getCmp(BasicBlock *BB)
{
    assert(BB->get_last_instrution()->isBranch());
    Branch *p_branch = (Branch *)BB->get_last_instrution();
    return (Cmp *)p_branch->get_cond();
}

static inline void _DealHeader(Loop *loop, BasicBlock *workBB, IRCopy &copyer)
{
    BasicBlock *prev = loop->get_prev();
    assert(prev->get_last_instrution()->isJmp());
    Cmp *p_cmp = _getCmp(workBB);
    Instrution *p_br = prev->get_last_instrution();
    prev->get_instrs()->pop_back();
    Cmp *nwCmp = new Cmp(p_cmp->get_Instrtype(), p_cmp->get_src1(), p_cmp->get_src2(), prev);
    auto list = loop->get_header()->get_value_list();
    for (auto it = list->begin(); it != list->end(); ++it)
    {
        if ((*it)->get_val() == prev)
        {
            list->erase(it);
            break;
        }
    }
    list = copyer.get_mapbb(loop->get_header())->get_value_list();
    for (auto it = list->begin(); it != list->end(); ++it)
    {
        if ((*it)->get_val() == copyer.get_mapbb(prev))
        {
            list->erase(it);
            break;
        }
    }
    delete (*prev->get_user_list())[0];
    prev->get_user_list()->clear();
    new Branch(nwCmp, loop->get_header(), copyer.get_mapbb(loop->get_header()), prev);
    p_br->print();
    assert(prev->get_last_instrution()->isBranch());
    BasicBlock *CBB = copyer.get_mapbb(workBB);
    Branch *p_branch = (Branch *)workBB->get_last_instrution();
    workBB->Ins_popBack();
    Cmp *p_cond = (Cmp *)p_branch->get_cond();
    Branch *Cp_brach = (Branch *)copyer.get_mapval(p_branch);
    CBB->Ins_popBack();
    Cmp *Cp_cond = (Cmp *)copyer.get_mapval(p_cond);
    Constant *const0 = new ConstantI32(0);
    workBB->get_func()->value_pushBack(const0);
    for (auto it = p_cond->get_user_list()->begin(); it != p_cond->get_user_list()->end(); ++it)
    {
        if ((*it)->get_user() == p_branch)
        {
            Cmp *nwCmp = new Cmp(InstrutionEnum::IEQ, const0, const0, workBB);
            (*it)->set_val(nwCmp);
            p_cond->get_user_list()->erase(it);
            break;
        }
    }
    for (auto it = Cp_cond->get_user_list()->begin(); it != Cp_cond->get_user_list()->end(); ++it)
    {
        if ((*it)->get_user() == Cp_brach)
        {
            Cmp *nwCmp = new Cmp(InstrutionEnum::INEQ, const0, const0, CBB);
            (*it)->set_val(nwCmp);
            Cp_cond->get_user_list()->erase(it);
            break;
        }
    }
    workBB->Ins_pushBack(p_branch);
    CBB->Ins_pushBack(Cp_brach);
}

void loopunswitch::Unswitch(Loop *loop, Function *p_func, BasicBlock *workBB, DomTree &domtree)
{
    IRCopy copyer;
    Function *Cfunc = copyer.copy_func(p_func);
    std::set<Value *> _addValues;
    std::vector<BasicBlock *> _addBBs;
    for (BasicBlock *_BB : *loop->get_BBs())
    {
        _addBBs.emplace_back(copyer.get_mapbb(_BB));
        for (PHINode *_Phi : *_BB->get_phinodes())
        {
            _addValues.insert(copyer.get_mapval(_Phi));
            _dealVal(_Phi, loop, _addValues, copyer);
            _dealUser(_Phi, loop, copyer, domtree);
        }
        std::vector<Instrution *> *instrs = _BB->get_instrutions();
        for (Instrution *p_instr : *instrs)
        {
            _addValues.insert(copyer.get_mapval(p_instr));
            _dealVal(p_instr, loop, _addValues, copyer);
            _dealUser(p_instr, loop, copyer, domtree);
        }
    }
    puts("cppyer");
    for (BasicBlock *_BB : *loop->get_BBs())
    {
        copyer.get_mapbb(_BB)->print();
    }
    _Dealexits(loop, copyer);

    auto nwblocks = Cfunc->get_blocks();
    for (BasicBlock *addBB : _addBBs)
    {
        nwblocks->erase(std::remove(nwblocks->begin(), nwblocks->end(), addBB), nwblocks->end());
        p_func->block_pushBack(addBB);
        addBB->Set_parent(p_func);
    }
    auto nwValues = Cfunc->get_Values();
    for (Value *addval : _addValues)
    {
        nwValues->erase(std::remove(nwValues->begin(), nwValues->end(), addval), nwValues->end());
        p_func->value_pushBack(addval);
    }
    _DealHeader(loop, workBB, copyer);
    delete Cfunc;
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

static inline BasicBlock *checkIF(Loop *loop)
{
    for (BasicBlock *BB : *loop->get_BBs())
    {
        Instrution *p_branch = BB->get_last_instrution();
        if (!p_branch->isBranch())
            continue;
        Cmp *p_cond = (Cmp *)((Branch *)p_branch)->get_cond();
        if (_CheckVar(p_cond->get_src1(), loop) && _CheckVar(p_cond->get_src2(), loop))
            return BB;
    }
    return nullptr;
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

void loopunswitch::FuncDealer(Function *p_func)
{

    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    std::cout << p_func->get_name() << std::endl;
    LCSSA worker(p_func, _Loop);
    worker.run();
    _checkloop(loop);
    DomTree domtree(p_func);
    domtree.Run();
    for (Loop *son : *loop->get_lpsons())
    {
        bool flag = false;
        searchFunc(son, p_func, domtree, flag);
    }
}

void loopunswitch::PassRun(Module *p_module)
{
    puts("\n Unswitch\n");
    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    p_module->print();
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
    p_module->print();
}

loopunswitch::loopunswitch()
{
    _SCEV = new SCEV();
}
loopunswitch::~loopunswitch()
{
    delete _SCEV;
}
