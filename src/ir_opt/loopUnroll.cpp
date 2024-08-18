#include <algorithm>

#include "../../include/ir_opt/loopUnroll.hpp"

std::set<BasicBlock *> *loopUnroll::BBmap = new std::set<BasicBlock *>;

#define MustDo(it, list_) ((it) == (list_)->end() ? Assert_() : true)

static inline void ResetVal_(PHINode *phi, Value *val, BasicBlock *BB, IRCopy &copyer)
{
    PHINode *cphi = (PHINode *)copyer.get_mapval(phi);
    BasicBlock *cBB = copyer.get_mapbb(BB);
    cphi->eraseIncoming(cBB);
    cphi->addIncoming(val, cBB);
}

void loopUnroll::SetVal(PHINode *phi, IRCopy &copyer, Loop *loop)
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

void loopUnroll::SetUser(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list_ = instr->get_user_list();
    for (auto it_ : *list_)
    {
        Instrution *T = (Instrution *)it_->get_user();
        BasicBlock *pBB = T->get_parent();
        if (loop->is_BBinLoop(pBB))
            continue;
        assert(is_a<PHINode>(it_->get_user()));
        PHINode *user = (PHINode *)it_->get_user();
        BasicBlock *BB = user->get_edge_income_block(it_);
        assert(loop->is_BBinLoop(BB));
        PHINode *cuser = (PHINode *)copyer.get_mapval(user);
        BasicBlock *cBB = copyer.get_mapbb(BB);
        cuser->eraseIncoming(cBB);
        user->addIncoming(copyer.get_mapval(instr), cBB);
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

void loopUnroll::SetVal(Instrution *instr, IRCopy &copyer, Loop *loop)
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

static inline void _dealCond(BasicBlock *_exiting, BasicBlock *nextBB, Loop *loop, BasicBlock *nwexiting)
{
    assert(nwexiting->get_last_instrution()->isBranch());
    Branch *p_branch = ((Branch *)nwexiting->get_last_instrution());
    nwexiting->get_instrs()->pop_back();
    Cmp *p_cond = (Cmp *)p_branch->get_cond();
    p_cond->drop();

    Constant *const1 = new ConstantI32(1);
    _exiting->get_func()->value_pushBack(const1);
    Edge *newEdge = new Edge(nextBB, nwexiting);
    nwexiting->get_user_list()->pop_back();

    Edge *delEdge = nullptr;
    Branch *tmp = nullptr;
    if (nwexiting == _exiting)
        tmp = p_branch;
    else
        tmp = (Branch *)_exiting->get_last_instrution();

    if (loop->is_BBinLoop(tmp->get_trueBB()))
    {
        p_cond = new Cmp(InstrutionEnum::INEQ, const1, const1, nwexiting);
        delEdge = (*nwexiting->get_user_list())[1];
        (*nwexiting->get_user_list())[1] = newEdge;
    }
    else
    {
        p_cond = new Cmp(InstrutionEnum::IEQ, const1, const1, nwexiting);
        delEdge = (*nwexiting->get_user_list())[0];
        (*nwexiting->get_user_list())[0] = newEdge;
    }

    auto DelEdges = delEdge->get_user()->get_value_list();
    for (auto it = DelEdges->begin(); it != DelEdges->end(); ++it)
        if ((*it) == delEdge)
        {
            DelEdges->erase(it);
            break;
        }
    delete delEdge;

    new Edge(p_branch, p_cond);
    nwexiting->get_instrs()->push_back(p_branch);
    assert(p_branch->get_value_list()->size() == 1);
}

static inline void _dealLast(Loop *loop, IRCopy &Fcopyer, IRCopy &Lcopyer)
{
    BasicBlock *_exiting = *loop->get_exitings()->begin();
    BasicBlock *_Lexiting = Lcopyer.get_mapbb(_exiting);
    BasicBlock *_exit = *loop->get_exits()->begin();

    BasicBlock *p_true = nullptr, *p_false = nullptr;
    if (loop->is_BBinLoop((BasicBlock *)((*_exiting->get_user_list()->begin())->get_user()))) //???
    {
        p_true = Fcopyer.get_mapbb(loop->get_header());
        p_false = _exit;
    }
    else
    {
        p_true = _exit;
        p_false = Fcopyer.get_mapbb(loop->get_header());
    }
    assert(p_true && p_false);

    BasicBlock *_latch = *loop->get_latchs()->begin();
    for (PHINode *Phi : *loop->get_header()->get_phinodes())
    {
        PHINode *_NwPhi = (PHINode *)Fcopyer.get_mapval(Phi);
        PHINode *_BBPhi = (PHINode *)Lcopyer.get_mapval(Phi);
        _NwPhi->addIncoming(Lcopyer.get_mapval(Phi->get_valueMap()->find(_latch)->second->get_val()), _Lexiting);
        _BBPhi->eraseIncoming(Lcopyer.get_mapbb(_latch));
    }
    for (int i = 0; i < _Lexiting->get_user_list()->size(); ++i)
    {
        Value *_user = (*_Lexiting->get_user_list())[i]->get_user();
        for (auto it = _user->get_value_list()->begin(); it != _user->get_value_list()->end(); ++it)
        {
            if ((*it)->get_val() == _Lexiting)
            {
                _user->get_value_list()->erase(it);
                break;
            }
        }
        delete (*_Lexiting->get_user_list())[i];
    }
    _Lexiting->get_user_list()->clear();
    new Edge(p_true, _Lexiting);
    new Edge(p_false, _Lexiting);
    assert(_Lexiting->get_user_list()->size() == 2);
}

static inline void _AddLoopIf(Loop *loop, const int times, SCEV *_SCEV, BasicBlock *_Judge)
{
    BasicBlock *prev = loop->get_prev();
    prev->get_instrs()->pop_back();
    puts("Step");
    SCEVEXP *p_exp = _SCEV->find_exp(loop->get_lpStep());
    assert(p_exp != nullptr && !(*p_exp->get_dims())[0].empty() && !(*p_exp->get_dims())[1].empty() && (*p_exp->get_dims())[2].empty());
    Value *src0 = p_exp->get_scr(0, prev);
    Value *src1 = p_exp->get_scr(1, prev);
    Value *End = loop->get_lpEnd();
    assert(src0 && src1);
    bool is_F32 = false;
    if (src0->get_type()->get_type() == TypeEnum::F32 || End->get_type()->get_type() == TypeEnum::F32)
    {
        is_F32 = true;
        if (src0->get_type()->get_type() == TypeEnum::I32)
        {
            src0 = new Unary(InstrutionEnum::I2F, src0, prev);
            src1 = new Unary(InstrutionEnum::I2F, src1, prev);
        }
        else if (End->get_type()->get_type() == TypeEnum::I32)
            End = new Unary(InstrutionEnum::I2F, End, prev);
    }
    Instrution *p_instr = nullptr;
    Constant *const1 = nullptr;
    Value *p_offset = nullptr;
    int _type = loop->get_cmpType();
    switch (_type)
    {
    case 1:
        if (is_F32)
            const1 = new ConstantF32(1.0);
        else
            const1 = new ConstantI32(1);
        prev->get_func()->value_pushBack(const1);
        End = new Binary(is_F32 == true ? InstrutionEnum::FSUB : InstrutionEnum::ISUB, End, const1, prev);
    case 0:
        if (is_F32)
            const1 = new ConstantF32(1.0);
        else
            const1 = new ConstantI32(1);
        prev->get_func()->value_pushBack(const1);
        p_offset = new Binary(is_F32 == true ? InstrutionEnum::FADD : InstrutionEnum::IADD, src1, const1, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FSUB : InstrutionEnum::ISUB, End, src0, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FADD : InstrutionEnum::IADD, p_instr, p_offset, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FDIV : InstrutionEnum::IDIV, p_instr, src1, prev);
        break;

    case 3:
        if (is_F32)
            const1 = new ConstantF32(1.0);
        else
            const1 = new ConstantI32(1);
        prev->get_func()->value_pushBack(const1);
        End = new Binary(is_F32 == true ? InstrutionEnum::FADD : InstrutionEnum::IADD, End, const1, prev);
    case 2:
        if (is_F32)
            const1 = new ConstantF32(1.0);
        else
            const1 = new ConstantI32(1);
        prev->get_func()->value_pushBack(const1);
        p_offset = new Binary(is_F32 == true ? InstrutionEnum::FSUB : InstrutionEnum::ISUB, src1, const1, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FSUB : InstrutionEnum::ISUB, End, src0, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FADD : InstrutionEnum::IADD, p_instr, p_offset, prev);
        p_instr = new Binary(is_F32 == true ? InstrutionEnum::FDIV : InstrutionEnum::IDIV, p_instr, src1, prev);
        break;
    default:
        assert(0);
        break;
    }
    if (is_F32)
        p_instr = new Unary(InstrutionEnum::F2I, p_instr, prev);
    const1 = new ConstantI32(times);
    prev->get_func()->value_pushBack(const1);
    Instrution *p_Mod = new Binary(InstrutionEnum::IMOD, p_instr, const1, prev);
    // const1 = new ConstantI32(1);
    // prev->get_func()->value_pushBack(const1);
    // Instrution *_Step = new Binary(InstrutionEnum::ISUB, p_Mod, const1, prev);
    // if (is_F32)
    //     _Step = new Unary(InstrutionEnum::I2F, _Step, prev);
    Instrution *_nwEnd = new Binary(is_F32 ? InstrutionEnum::FMUL : InstrutionEnum::IMUL, p_Mod, src1, prev);
    _nwEnd = new Binary(is_F32 ? InstrutionEnum::FADD : InstrutionEnum::IADD, src0, _nwEnd, prev);
    Cmp *p_cmp = (Cmp *)loop->get_lpCmp();
    switch (p_cmp->get_Instrtype())
    {
    case InstrutionEnum::ILE:
        p_cmp->set_instrType(InstrutionEnum::ILT);
        break;
    case InstrutionEnum::IGE:
        p_cmp->set_instrType(InstrutionEnum::IGT);
        break;
    case InstrutionEnum::FLE:
        p_cmp->set_instrType(InstrutionEnum::FLT);
        break;
    case InstrutionEnum::FGE:
        p_cmp->set_instrType(InstrutionEnum::FGT);
        break;

    default:
        break;
    }
    for (auto it = p_cmp->get_value_list()->begin(); it != p_cmp->get_value_list()->end(); ++it)
    {
        if ((*it)->get_val() == loop->get_lpEnd())
        {
            std::vector<Edge *> *Edges = (*it)->get_val()->get_user_list();
            for (auto i = Edges->begin(); i != Edges->end(); ++i)
            {
                if ((*i) == (*it))
                {
                    Edges->erase(i);
                    break;
                }
            }
            (*it)->set_val(_nwEnd);
            break;
        }
    }
    assert(prev->get_user_list()->size() == 1);
    std::vector<Edge *> *_edges = (*prev->get_user_list()->begin())->get_user()->get_value_list();
    for (auto it = _edges->begin(); it != _edges->end(); ++it)
    {
        if ((*it)->get_val() == prev)
        {
            _edges->erase(it);
            break;
        }
    }
    delete (*prev->get_user_list()->begin());
    prev->get_user_list()->clear();
    const1 = new ConstantI32(0);
    prev->get_func()->value_pushBack(const1);
    Instrution *_cond = new Cmp(InstrutionEnum::IEQ, p_Mod, const1, prev);
    prev->Set_branch(_cond, _Judge, loop->get_header());
}

static inline void CutUser(Instrution *instr, IRCopy &copyer, Loop *loop)
{
    auto list = instr->get_user_list();
    for (auto it1 = list->begin(); it1 != list->end(); ++it1)
    {
        Instrution *T = (Instrution *)(*it1)->get_user();
        BasicBlock *pBB = T->get_parent();
        if (loop->is_BBinLoop(pBB))
            continue;
        auto Clist = copyer.get_mapval(instr)->get_user_list();
        Value *delVal = copyer.get_mapval(T);
        for (Edge *edge : *Clist)
            if (delVal == edge->get_user())
            {
                edge->drop();
                break;
            }
    }
}

void loopUnroll::Unroll(Loop *loop)
{
    assert(loop->get_BBs()->size() == 1);
    Function *p_func = loop->get_header()->get_func();
    std::vector<IRCopy> copyer(times);
    std::vector<Function *> cFunc(times);
    for (int i = 0; i < times; ++i)
        cFunc[i] = copyer[i].copy_func(p_func);

    BBmap->insert(loop->get_header());
    BBmap->insert(copyer[0].get_mapbb(loop->get_header()));
    int subTimes = times - 1;
    for (int i = 0; i < times; ++i)
    {
        std::vector<Value *> addvals;
        std::vector<BasicBlock *> addBBs;
        for (BasicBlock *BB : *loop->get_BBs())
        {
            printf("i = %d\n", i);
            addBBs.emplace_back(copyer[i].get_mapbb(BB));
            for (PHINode *phi : *BB->get_phinodes())
            {
                addvals.emplace_back(copyer[i].get_mapval(phi));
                SetVal(phi, copyer[i], loop);
                if (i == subTimes)
                    SetUser(phi, copyer[i], loop);
                else
                    CutUser(phi, copyer[i], loop);
            }
            for (Instrution *instr : *BB->get_instrutions())
            {
                addvals.emplace_back(copyer[i].get_mapval(instr));
                SetVal(instr, copyer[i], loop);
                if (i == subTimes)
                    SetUser(instr, copyer[i], loop);
                else
                    CutUser(instr, copyer[i], loop);
            }
        }
        for (auto it : addBBs)
        {
            auto list_ = cFunc[i]->get_blocks();
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
            auto list_ = cFunc[i]->get_Values();
            for (auto del = list_->begin(); MustDo(del, list_); ++del)
                if ((*del) == it)
                {
                    p_func->get_Values()->emplace_back(it);
                    list_->erase(del);
                    break;
                }
        }
    }
    puts("deal cond");

    for (int i = 0; i < subTimes; ++i)
    {
        BasicBlock *exitingBB = *loop->get_exitings()->begin();
        BasicBlock *CexitingBB = copyer[i].get_mapbb(exitingBB);
        BasicBlock *CnextBB = copyer[i + 1].get_mapbb(loop->get_header());
        _dealCond(exitingBB, CnextBB, loop, CexitingBB);
        BasicBlock *CexitBB = copyer[i].get_mapbb(*loop->get_exits()->begin());
        for (PHINode *Phi : *CexitBB->get_phinodes())
            Phi->eraseIncoming(CexitingBB);

        for (PHINode *phi : *loop->get_header()->get_phinodes())
        {
            PHINode *Cphi = (PHINode *)copyer[i].get_mapval(phi);
            Cphi->eraseIncoming(copyer[i].get_mapbb(loop->get_prev()));

            PHINode *CNphi = (PHINode *)copyer[i + 1].get_mapval(phi);
            assert(Cphi->get_valueMap()->find(copyer[i].get_mapbb(*loop->get_latchs()->begin())) != Cphi->get_valueMap()->end());
            CNphi->addIncoming(Cphi->get_valueMap()->find(copyer[i].get_mapbb(*loop->get_latchs()->begin()))->second->get_val(),
                               copyer[i].get_mapbb(*loop->get_exitings()->begin()));
        }

        BasicBlock *Cprev = copyer[i].get_mapbb(loop->get_prev());
        (*Cprev->get_user_list()->begin())->drop();
    }

    {
        _dealLast(loop, copyer[0], copyer[subTimes]);

        for (PHINode *phi : *loop->get_header()->get_phinodes())
        {
            PHINode *Cphi = (PHINode *)copyer[subTimes].get_mapval(phi);
            Cphi->eraseIncoming(copyer[subTimes].get_mapbb(loop->get_prev()));
        }
        BasicBlock *Cprev = copyer[subTimes].get_mapbb(loop->get_prev());
        (*Cprev->get_user_list()->begin())->drop();
    }

    for (int i = 0; i < times; ++i)
    {
        cFunc[i]->DeleteGVals();
        delete cFunc[i];
    }

    BasicBlock *_Judge = new BasicBlock(p_func);
    p_func->block_pushBack(_Judge);
    // set JBB branch
    Cmp *_LPCmp = (Cmp *)loop->get_lpCmp();
    Cmp *_JudgeCmp = new Cmp(_LPCmp->get_Instrtype(), _LPCmp->get_src1(), _LPCmp->get_src2(), _Judge);
    Value *src1 = _LPCmp->get_src1(), *src2 = _LPCmp->get_src2();
    BasicBlock *_TBB = *loop->get_exits()->begin(), *_FBB = copyer[0].get_mapbb(loop->get_header());
    BasicBlock *lpexiting = *loop->get_exitings()->begin();
    BasicBlock *lplatch = *loop->get_latchs()->begin();
    if (loop->is_BBinLoop((BasicBlock *)((*lpexiting->get_user_list()->begin())->get_user())))
        std::swap(_TBB, _FBB);
    _Judge->Set_branch(_JudgeCmp, _TBB, _FBB);

    _AddLoopIf(loop, times, _SCEV, _Judge);
    BasicBlock *CheaderBB = copyer[0].get_mapbb(loop->get_header());

    for (PHINode *Phi : *loop->get_header()->get_phinodes())
    {
        PHINode *_nextPhi = ((PHINode *)copyer[0].get_mapval(Phi));

        PHINode *NwPhi = new PHINode(_Judge, _nextPhi->get_type()->get_type(), true);
        _Judge->Insert_Phi(NwPhi);
        /*
        std::vector<Edge *> *Edges = Phi->get_valueMap()->find(lplatch)->second->get_val()->get_user_list();
        std::vector<Edge *> _Del;
        assert(_Del.empty());
        for (auto it = Edges->begin(); it != Edges->end(); ++it)
        {
            if (loop->is_BBinLoop(((Instrution *)(*it)->get_user())->get_parent()) && ((Instrution *)(*it)->get_user())->get_parent() != _PhiBB)
                continue;
            _Del.push_back(*it);
            (*it)->set_val(NwPhi);
        }
        for (auto it : _Del)
            Edges->erase(std::remove(Edges->begin(), Edges->end(), it), Edges->end());
        */
        NwPhi->addIncoming(Phi->get_valueMap()->find(loop->get_prev())->second->get_val(), loop->get_prev());
        NwPhi->addIncoming(Phi->get_valueMap()->find(lplatch)->second->get_val(), (*loop->get_exitings()->begin()));
        _nextPhi->addIncoming(NwPhi, _Judge);
    }

    {
        Instrution *p_branch = _Judge->get_last_instrution();
        _Judge->Ins_popBack();
        Cmp *cmp = (Cmp *)_Judge->get_last_instrution();
        cmp->drop();
        bool flag = false;
        for (PHINode *phi : *_Judge->get_phinodes())
        {
            auto vals = phi->get_value_list();
            for (Edge *edge : *vals)
            {
                Value *val = edge->get_val();
                if (val == src1)
                {
                    src1 = phi;
                    flag = true;
                    break;
                }
                else if (val == src2)
                {
                    src2 = phi;
                    flag = true;
                    break;
                }
            }
            if (flag)
                break;
        }
        cmp = new Cmp(_LPCmp->get_Instrtype(), src1, src2, _Judge);
        new Edge(p_branch, cmp);
        _Judge->Ins_pushBack(p_branch);
    }

    BasicBlock *_exit = *loop->get_exits()->begin();

    for (auto it = _exit->get_value_list()->begin(); it != _exit->get_value_list()->end(); ++it)
        if (((*it)->get_val() == lpexiting))
        {
            (*it)->set_user(_Judge);
            _exit->get_value_list()->erase(it);
            break;
        }

    BasicBlock *exitingBB = *loop->get_exitings()->begin();
    for (PHINode *phi : *_exit->get_phinodes())
    {
        Value *val = phi->get_valueMap()->find(exitingBB)->second->get_val();
        auto users = val->get_user_list();
        for (auto user : *users)
        {
            Instrution *instr = (Instrution *)user->get_user();
            if (instr->get_parent() == _Judge && instr->isPHINode())
            {
                val = instr;
                break;
            }
        }
        phi->eraseIncoming(exitingBB);
        phi->addIncoming(val, _Judge);
    }

    { // opt if branch
        for (PHINode *phi : *_Judge->get_phinodes())
        {
            Value *val = phi->get_valueMap()->find(loop->get_prev())->second->get_val();
            auto list_ = phi->get_user_list();
            bool phiFlag = true;
            for (Edge *edge : *list_)
            {
                Instrution *user = (Instrution *)edge->get_user();
                if (user->isPHINode() && user->get_parent() == CheaderBB)
                {
                    assert(phiFlag);
                    PHINode *cPhi = (PHINode *)user;
                    cPhi->addIncoming(val, loop->get_prev());
                    phi->eraseIncoming(loop->get_prev());
                    phiFlag = false;
                }
            }
        }

        BasicBlock *prevBB = loop->get_prev();
        auto list_ = _Judge->get_value_list();
        for (auto it = list_->begin(); it != list_->end(); ++it)
            if ((*it)->get_val() == prevBB)
            {
                (*it)->set_user(CheaderBB);
                list_->erase(it);
                break;
            }
    }
}

bool loopUnroll::analysis(Loop *loop)
{
    BasicBlock *BB = loop->get_header();
    if (BB->get_instrs()->size() + BB->get_phinodes()->size() > 30 || BBmap->find(BB) != BBmap->end())
        return false;

    for (Instrution *it : *BB->get_instrs())
    {
        if (it->get_type()->get_type() == TypeEnum::VecI32)
            return false;
    }
    return true;
}

void loopUnroll::searchFunc(Loop *loop, Function *func)
{
    for (Loop *son : *loop->get_lpsons())
        searchFunc(son, func);
    if (!loop->get_lpsons()->empty() || !loop->is_stepSet() || flag)
        return;
    if (loop->get_BBs()->size() != 1)
        return;
    if (analysis(loop))
        Unroll(loop);
    flag = true;
}

void loopUnroll::FuncDealer(Function *p_func)
{
    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    LCSSA worker(p_func, _Loop);
    worker.run();

    for (Loop *son : *loop->get_lpsons())
    {
        flag = false;
        searchFunc(son, p_func);
    }
}

void loopUnroll::PassRun(Module *p_module)
{
    codesize = 0;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        codesize += p_func->get_Values()->size();
    }

    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}

loopUnroll::loopUnroll()
{
    _SCEV = new SCEV();
}
loopUnroll::~loopUnroll()
{
    delete _SCEV;
}
