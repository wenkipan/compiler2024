#include "../../include/ir_opt/loop.hpp"
#include <algorithm>

#define MustDo(it, list_) ((it) == (list_)->end() ? Assert_() : true)

static inline void ResetVal_(PHINode *phi, Value *val, BasicBlock *BB, IRCopy &copyer)
{
    PHINode *cphi = (PHINode *)copyer.get_mapval(phi);
    BasicBlock *cBB = copyer.get_mapbb(BB);
    cphi->eraseIncoming(cBB);
    cphi->addIncoming(val, cBB);
}

void loopFFF::SetVal(PHINode *phi, IRCopy &copyer, Loop *loop)
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

void loopFFF::SetVal(Instrution *instr, IRCopy &copyer, Loop *loop)
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
    Instrution *_cond = new Cmp(InstrutionEnum::ILE, p_Mod, const1, prev);
    prev->Set_branch(_cond, _Judge, loop->get_header());
}

BasicBlock *loopFFF::earlyBB(Loop *loop)
{
    if (loop->get_header() == nullptr)
        return nullptr;
    BasicBlock *re = nullptr;
    while (loop->get_header() != nullptr)
    {
        if (loop->hasCall())
            return re;
        re = loop->get_prev();
        loop = loop->get_parent();
    }
    return re;
}

void loopFFF::Unroll(Loop *loop)
{

    assert(loop->get_BBs()->size() == 1);
    Function *p_func = loop->get_header()->get_func();
    IRCopy copyer;
    Function *cFunc;
    cFunc = copyer.copy_func(p_func);

    {
        std::vector<Value *> addvals;
        std::vector<BasicBlock *> addBBs;
        for (BasicBlock *BB : *loop->get_BBs())
        {
            addBBs.emplace_back(copyer.get_mapbb(BB));
            for (PHINode *phi : *BB->get_phinodes())
            {
                addvals.emplace_back(copyer.get_mapval(phi));
                SetVal(phi, copyer, loop);
            }
            for (Instrution *instr : *BB->get_instrutions())
            {
                addvals.emplace_back(copyer.get_mapval(instr));
                SetVal(instr, copyer, loop);
            }
        }
        for (auto it : addBBs)
        {
            auto list_ = cFunc->get_blocks();
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
            auto list_ = cFunc->get_Values();
            for (auto del = list_->begin(); MustDo(del, list_); ++del)
                if ((*del) == it)
                {
                    p_func->get_Values()->emplace_back(it);
                    list_->erase(del);
                    break;
                }
        }
    }
    BasicBlock *header = loop->get_header();
    BasicBlock *cheader = copyer.get_mapbb(header);
    BasicBlock *newBB = new BasicBlock(p_func);
    p_func->block_pushBack(newBB);

    {

        if ((*header->get_user_list()->begin())->get_user() == header)
            (*copyer.get_mapbb(header)->get_user_list())[1]->set_user(*loop->get_exits()->begin());
        else
            (*copyer.get_mapbb(header)->get_user_list())[0]->set_user(*loop->get_exits()->begin());

        bool afterST = false;
        std::unordered_map<Instrution *, Instrution *> instrMap_;
        std::unordered_map<Instrution *, Instrution *> loadMap_;
        for (Instrution *originInstr : *header->get_instrs())
        {
            Instrution *instr = (Instrution *)copyer.get_mapval(originInstr);
            if (instr->isLoad())
            {
                Value *val = ((Load *)originInstr)->get_addr();
                bool ptrFlag = false;
                GEP *gep = dynamic_cast<GEP *>(val);
                if (gep != nullptr)
                {
                    if (loop->is_BBinLoop(gep->get_BB()))
                        ptrFlag = true;
                }
                else
                    ptrFlag = false;
                if (ptrFlag)
                    instr->get_type()->reset(TypeEnum::VecI32);
                else
                {
                    Assign *p_assign = new Assign(InstrutionEnum::Assign, instr, newBB, false);
                    p_assign->get_type()->reset(TypeEnum::VecI32);
                    auto list_ = instr->get_user_list();
                    bool invflag = true;
                    while (invflag)
                    {
                        invflag = false;
                        for (auto it = list_->begin(); it != list_->end(); ++it)
                        {
                            if ((*it)->get_user() != p_assign)
                            {
                                (*it)->set_val(p_assign);
                                list_->erase(it);
                                invflag = true;
                                break;
                            }
                        }
                    }
                    loadMap_.insert({p_assign, instr});
                }
            }

            else if (instr->isStore())
            {
                afterST = true;
                Value *val = ((Store *)instr)->get_src();
                if (val->get_type()->get_type() != TypeEnum::VecI32)
                {
                    BasicBlock *TBB = earlyBB(loop->get_parent());

                    Assign *p_assign = nullptr;

                    if (TBB == nullptr)
                        p_assign = new Assign(InstrutionEnum::Assign, val, newBB, false);
                    else if (TBB->get_last_instrution()->isBranch())
                    {
                        Instrution *p_branch = TBB->get_last_instrution();
                        TBB->Ins_popBack();
                        Instrution *p_cmp = TBB->get_last_instrution();
                        TBB->Ins_popBack();
                        p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                        TBB->Ins_pushBack(p_cmp);
                        TBB->Ins_pushBack(p_branch);
                    }
                    else if (TBB->get_last_instrution()->isJmp())
                    {
                        Instrution *p_branch = TBB->get_last_instrution();
                        TBB->Ins_popBack();
                        p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                        TBB->Ins_pushBack(p_branch);
                    }
                    else
                        assert(0);

                    p_assign->get_type()->reset(TypeEnum::VecI32);

                    auto list_ = val->get_user_list();
                    for (auto it = list_->begin(); it != list_->end(); ++it)
                    {
                        if ((*it)->get_user() == instr)
                        {
                            (*it)->set_val(p_assign);
                            list_->erase(it);
                        }
                    }
                    // instrMap_.insert({instr, p_assign});
                }
            }
            else if (instr->isBinary())
            {
                Binary *p_b = (Binary *)instr;
                if (p_b->get_src1()->get_type()->get_type() == TypeEnum::VecI32)
                {
                    if (p_b->get_src2()->get_type()->get_type() != TypeEnum::VecI32)
                    {
                        Value *val = p_b->get_src2();
                        if (val->get_type()->get_type() != TypeEnum::VecI32)
                        {
                            BasicBlock *TBB = earlyBB(loop->get_parent());

                            Assign *p_assign = nullptr;

                            if (TBB == nullptr)
                                p_assign = new Assign(InstrutionEnum::Assign, val, newBB, false);
                            else if (TBB->get_last_instrution()->isBranch())
                            {
                                Instrution *p_branch = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                Instrution *p_cmp = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                                TBB->Ins_pushBack(p_cmp);
                                TBB->Ins_pushBack(p_branch);
                            }
                            else if (TBB->get_last_instrution()->isJmp())
                            {
                                Instrution *p_branch = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                                TBB->Ins_pushBack(p_branch);
                            }
                            else
                                assert(0);

                            p_assign->get_type()->reset(TypeEnum::VecI32);

                            auto list_ = val->get_user_list();
                            for (auto it = list_->begin(); it != list_->end(); ++it)
                            {
                                if ((*it)->get_user() == instr)
                                {
                                    (*it)->set_val(p_assign);
                                    list_->erase(it);
                                }
                            }
                        }
                    }
                    p_b->get_type()->reset(TypeEnum::VecI32);
                }
                else if (p_b->get_src2()->get_type()->get_type() == TypeEnum::VecI32)
                {
                    if (p_b->get_src1()->get_type()->get_type() != TypeEnum::VecI32)
                    {
                        Value *val = p_b->get_src2();
                        if (val->get_type()->get_type() != TypeEnum::VecI32)
                        {
                            BasicBlock *TBB = earlyBB(loop->get_parent());

                            Assign *p_assign = nullptr;

                            if (TBB == nullptr)
                                p_assign = new Assign(InstrutionEnum::Assign, val, newBB, false);
                            else if (TBB->get_last_instrution()->isBranch())
                            {
                                Instrution *p_branch = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                Instrution *p_cmp = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                                TBB->Ins_pushBack(p_cmp);
                                TBB->Ins_pushBack(p_branch);
                            }
                            else if (TBB->get_last_instrution()->isJmp())
                            {
                                Instrution *p_branch = TBB->get_last_instrution();
                                TBB->Ins_popBack();
                                p_assign = new Assign(InstrutionEnum::Assign, val, TBB, false);
                                TBB->Ins_pushBack(p_branch);
                            }
                            else
                                assert(0);

                            p_assign->get_type()->reset(TypeEnum::VecI32);

                            auto list_ = val->get_user_list();
                            for (auto it = list_->begin(); it != list_->end(); ++it)
                            {
                                if ((*it)->get_user() == instr)
                                {
                                    (*it)->set_val(p_assign);
                                    list_->erase(it);
                                }
                            }
                        }
                    }
                    p_b->get_type()->reset(TypeEnum::VecI32);
                }
                else
                {
                    if (!afterST)
                        continue;
                    assert(p_b->get_user_list()->size() == 2);
                    auto edges = p_b->get_value_list();
                    for (Edge *edge : *edges)
                    {
                        Value *val = edge->get_val();
                        if (is_a<ConstantI32>(val))
                        {
                            auto dels = val->get_user_list();
                            for (auto it = dels->begin(); MustDo(it, dels); ++it)
                                if ((*it) == edge)
                                {
                                    dels->erase(it);
                                    break;
                                }
                            ConstantI32 *num_4 = new ConstantI32(4);
                            p_func->value_pushBack(num_4);
                            edge->set_val(num_4);
                        }
                    }
                }
            }
            else if (instr->isCmp() || instr->isBranch() || instr->isGEP())
                ;
            else
                assert(0);
        }

        for (auto it : instrMap_)
        {
            auto instrs = cheader->get_instrs();
            for (auto instr = instrs->begin(); MustDo(instr, instrs); ++instr)
            {
                if ((*instr) == it.first)
                {
                    instrs->insert(instr, it.second);
                    break;
                }
            }
        }

        for (auto it : loadMap_)
        {

            newBB->instr_insert_before(it.first, it.second);
        }
    }

    {
        BasicBlock *_Judge = new BasicBlock(p_func);
        p_func->block_pushBack(_Judge);
        // set JBB branch
        Cmp *_LPCmp = (Cmp *)loop->get_lpCmp();
        Cmp *_JudgeCmp = new Cmp(_LPCmp->get_Instrtype(), _LPCmp->get_src1(), _LPCmp->get_src2(), _Judge);
        Value *src1 = _LPCmp->get_src1(), *src2 = _LPCmp->get_src2();
        BasicBlock *_TBB = *loop->get_exits()->begin(), *_FBB = copyer.get_mapbb(loop->get_header());
        BasicBlock *lpexiting = *loop->get_exitings()->begin();
        BasicBlock *lplatch = *loop->get_latchs()->begin();
        if (loop->is_BBinLoop((BasicBlock *)((*lpexiting->get_user_list()->begin())->get_user())))
            std::swap(_TBB, _FBB);
        _Judge->Set_branch(_JudgeCmp, _TBB, _FBB);

        _AddLoopIf(loop, times, _SCEV, _Judge);

        BasicBlock *cprev = copyer.get_mapbb(loop->get_prev());

        for (PHINode *Phi : *loop->get_header()->get_phinodes())
        {
            PHINode *_nextPhi = ((PHINode *)copyer.get_mapval(Phi));

            PHINode *NwPhi = new PHINode(_Judge, _nextPhi->get_type()->get_type(), true);
            _Judge->Insert_Phi(NwPhi);

            NwPhi->addIncoming(Phi->get_valueMap()->find(loop->get_prev())->second->get_val(), loop->get_prev());
            NwPhi->addIncoming(Phi->get_valueMap()->find(lplatch)->second->get_val(), (*loop->get_exitings()->begin()));
            _nextPhi->addIncoming(NwPhi, _Judge);
            _nextPhi->eraseIncoming(cprev);
        }
        auto edges = copyer.get_mapbb(loop->get_header())->get_value_list();
        for (auto it = edges->begin(); MustDo(it, edges); ++it)
            if ((*it)->get_val() == cprev)
            {
                edges->erase(it);
                break;
            }
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

        {
            auto list_ = (*loop->get_exits()->begin())->get_value_list();
            for (auto it = list_->begin(); it != list_->end(); ++it)
                if ((*it)->get_val() == lpexiting)
                {
                    (*it)->set_user(_Judge);
                    list_->erase(it);
                }
        }

        newBB->Set_jmp(cheader);
        auto list_ = _Judge->get_user_list();
        for (Edge *edge : *list_)
            if (edge->get_user() == cheader)
            {
                auto vals = cheader->get_value_list();
                for (auto it = vals->begin(); MustDo(it, vals); ++it)
                    if ((*it) == edge)
                    {
                        vals->erase(it);
                        break;
                    }
                edge->set_user(newBB);
            }
        for (PHINode *phi : *cheader->get_phinodes())
        {
            Value *val = phi->get_valueMap()->find(_Judge)->second->get_val();
            phi->eraseIncoming(_Judge);
            phi->addIncoming(val, newBB);
        }
    }

    cFunc->DeleteGVals();
    delete cFunc;
}

static inline bool checkType(Value *val, InstrutionEnum type)
{
    Instrution *instr = dynamic_cast<Instrution *>(val);
    assert(instr != nullptr);
    if (instr->get_Instrtype() != type)
        return false;
    return true;
}

static bool dfs(Value *val)
{
    if (is_a<ConstantF32>(val))
        return true;
    if (is_a<ConstantI32>(val) || is_a<Param>(val))
        return false;
    Instrution *instr = dynamic_cast<Instrution *>(val);
    assert(instr != nullptr);
    if (instr->isLoad())
        return false;
    if (instr->isPHINode())
        return true;
    assert(instr->isBinary());
    Binary *p_b = (Binary *)instr;
    return (dfs(p_b->get_src1()) || dfs(p_b->get_src2()));
}

static Value *ptrFind(GEP *gep)
{
    Value *ptr = gep->get_addr();
    GEP *src = dynamic_cast<GEP *>(ptr);
    if (src == nullptr)
        return ptr;
    else
        return ptrFind(src);
}

bool loopFFF::analysis(Loop *loop)
{
    BasicBlock *BB = loop->get_header();
    {
        auto instrs = loop->get_header()->get_instrs();
        int n = instrs->size();
        if (n <= 3)
            return false;
        assert((*instrs)[n - 1]->isBranch());
        SCEVEXP *p_exp = _SCEV->find_exp(*BB->get_phinodes()->begin());
        if (p_exp == nullptr)
            return false;
        auto dims = p_exp->get_dims();
        if ((*dims)[0].size() != 1 || (*dims)[1].size() != 1)
            return false;
        if (!is_a<ConstantI32>((*dims)[1][0].first))
            return false;
        ConstantI32 *c1 = (ConstantI32 *)(*dims)[1][0].first;
        if (c1->get_32_at(0) != 1)
            return false;
        if (!checkType((*instrs)[n - 2], InstrutionEnum::ILE) && !checkType((*instrs)[n - 2], InstrutionEnum::ILT))
            return false;
        if (!(*instrs)[n - 3]->isBinary())
            return false;
        auto list_ = (*instrs)[n - 3]->get_user_list();
        if (list_->size() != 2)
            return false;
        if (checkType((*list_)[0]->get_user(), InstrutionEnum::PHINode))
        {
            if (!((Instrution *)(*list_)[1]->get_user())->isCmp())
                return false;
        }
        else if (checkType((*list_)[1]->get_user(), InstrutionEnum::PHINode))
        {
            if (!((Instrution *)(*list_)[0]->get_user())->isCmp())
                return false;
        }
        else
            return false;
    }

    if (BB->get_phinodes()->size() == 1)
    {
        {
            auto instrs = loop->get_header()->get_instrs();
            if (!(*instrs)[instrs->size() - 4]->isStore())
                return false;
        }

        for (PHINode *phi : *BB->get_phinodes())
        {
            auto edges = phi->get_user_list();
            for (Edge *edge : *edges)
            {
                Instrution *instr = (Instrution *)edge->get_user();
                if (!loop->is_BBinLoop(instr->get_parent()))
                    return false;
            }
            if (phi->get_type()->get_type() == TypeEnum::F32)
                return false;
        }
        Value *src = nullptr;
        Value *ptr = nullptr;
        int n = BB->get_instrs()->size() - 3;
        std::set<Value *> ptrSet;
        for (int i = 0; i < n; ++i)
        {
            Instrution *it = (*BB->get_instrs())[i];
            if (it->get_type()->get_type() == TypeEnum::F32)
                return false;
            if (it->isCall() || it->get_type()->get_type() == TypeEnum::F32 || it->isUnary())
                return 0;
            switch (it->get_Instrtype())
            {
            case InstrutionEnum::IDIV:
            case InstrutionEnum::IMOD:
                return 0;
            case InstrutionEnum::GEP:
                ptr = ptrFind((GEP *)it);
                if (ptrSet.find(ptr) != ptrSet.end())
                    return false;
                ptrSet.insert(ptr);
                break;
            case InstrutionEnum::Store:
                if (src != nullptr)
                    return false;
                src = ((Store *)it)->get_src();
                break;
            default:
                break;
            }
            auto edges = it->get_user_list();
            for (Edge *edge : *edges)
            {
                Instrution *instr = (Instrution *)edge->get_user();
                if (loop->is_BBinLoop(instr->get_parent()))
                    continue;
                return 0;
            }
        }
        if (dfs(src))
            return false;
    }
    else
        return false;
    return 1;
}

void loopFFF::searchFunc(Loop *loop, Function *func)
{
    for (Loop *son : *loop->get_lpsons())
        searchFunc(son, func);
    if (!loop->get_lpsons()->empty() || !loop->is_stepSet())
        return;
    if (loop->get_BBs()->size() != 1)
        return;
    if (analysis(loop))
        Unroll(loop);
}

void loopFFF::FuncDealer(Function *p_func)
{

    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    for (Loop *son : *loop->get_lpsons())
        searchFunc(son, p_func);
}

void loopFFF::PassRun(Module *p_module)
{
    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}

loopFFF::loopFFF()
{
    _SCEV = new SCEV();
}
loopFFF::~loopFFF()
{
    delete _SCEV;
}

// loopYYY

int static inline _getd(Loop *loop)
{
    printf("Header b%d", loop->get_header()->get_ID());
    int re = 1;
    while (1)
    {
        if (loop->get_lpsons()->empty())
        {
            printf(":: %d\n", re);
            return re;
        }

        else if (loop->get_lpsons()->size() == 1)
        {
            re++;
            loop = *loop->get_lpsons()->begin();
        }
        else
            break;
    }
    return -1;
}

void loopYYY::W1(Loop *loop)
{
    if (loop->get_BBs()->size() != 1)
        return;
    BasicBlock *BB = loop->get_header();
    if (BB->get_phinodes()->size() != 1)
        return;
    if (!loop->is_stepSet() || BB->get_instrs()->size() != 5)
        return;
    auto instrs = BB->get_instrs();
    SCEVEXP *p_exp = scev->find_exp(*BB->get_phinodes()->begin());
    if (p_exp == nullptr)
        return;
    auto dims = p_exp->get_dims();
    if ((*dims)[0].size() != 1 || (*dims)[1].size() != 1)
        return;
    if (!is_a<ConstantI32>((*dims)[1][0].first))
        return;
    ConstantI32 *c1 = (ConstantI32 *)(*dims)[1][0].first;
    if (c1->get_32_at(0) != 1)
        return;
    Cmp *p_cmp = (Cmp *)(loop->get_lpCmp());
    if (p_cmp->get_Instrtype() != InstrutionEnum::ILT)
        return;
    BasicBlock *prevBB = loop->get_prev();
    PHINode *prevphi = (PHINode *)(*dims)[0][0].first;
    if (prevphi->get_valueMap()->size() != 1)
        return;
    if (!is_a<ConstantI32>((prevphi->get_valueMap()->begin()->second->get_val())))
        return;
    c1 = (ConstantI32 *)(prevphi->get_valueMap()->begin()->second->get_val());
    if (c1->get_32_at(0) != 0)
        return;
    int pos = 0;
    if (!(*instrs)[pos]->isGEP() || !(*instrs)[pos + 1]->isStore())
        return;
    GEP *gep = (GEP *)(*instrs)[pos];
    Store *ST = (Store *)(*instrs)[pos + 1];
    Value *addr = gep->get_addr();
    if (is_a<Instrution>(addr))
        if (loop->is_BBinLoop((((Instrution *)addr)->get_parent())))
            return;
    if (!is_a<ConstantI32>(ST->get_src()))
        return;
    Instrution *p_branch = prevBB->get_last_instrution();
    prevBB->Ins_popBack();
    ConstantI32 *const4 = new ConstantI32(4);
    BB->get_func()->value_pushBack(const4);
    Binary *p_mul = new Binary(InstrutionEnum::IMUL, p_cmp->get_src2(), const4, prevBB);
    Call *p_call = new Call(p_set, prevBB);
    p_call->params_pushback(addr);
    p_call->params_pushback(ST->get_src());
    p_call->params_pushback(p_mul);
    prevBB->Ins_pushBack(p_branch);
    (*instrs)[pos + 1]->drop();
}
void loopYYY::W2(Loop *loop)
{
    Loop *lson = *loop->get_lpsons()->begin();
    if (loop->get_BBs()->size() != 5 || lson->get_BBs()->size() != 1)
        return;
    BasicBlock *Fheader = loop->get_header(), *Sheader = lson->get_header();
    if (Fheader->get_phinodes()->size() != 1 || Sheader->get_phinodes()->size() != 1)
        return;
    if (!loop->is_stepSet() || !lson->is_stepSet())
        return;
    Cmp *F_cmp = (Cmp *)(loop->get_lpCmp()), *S_cmp = (Cmp *)(lson->get_lpCmp());
    if (F_cmp->get_Instrtype() != InstrutionEnum::ILT || S_cmp->get_Instrtype() != InstrutionEnum::ILT)
        return;
    SCEVEXP *p_exp = scev->find_exp(*Fheader->get_phinodes()->begin());
    if (p_exp == nullptr)
        return;
    auto dims = p_exp->get_dims();
    if ((*dims)[0].size() != 1 || (*dims)[1].size() != 1)
        return;
    if (!is_a<ConstantI32>((*dims)[1][0].first))
        return;
    ConstantI32 *c1 = (ConstantI32 *)(*dims)[1][0].first;
    if (c1->get_32_at(0) != 1)
        return;
    p_exp = scev->find_exp(*Sheader->get_phinodes()->begin());
    if (p_exp == nullptr)
        return;
    dims = p_exp->get_dims();
    if ((*dims)[0].size() != 1 || (*dims)[1].size() != 1)
        return;
    if (!is_a<ConstantI32>((*dims)[1][0].first))
        return;
    c1 = (ConstantI32 *)(*dims)[1][0].first;
    if (c1->get_32_at(0) != 1)
        return;

    auto instrs = Sheader->get_instrs();
    int pos = 0;
    if (!(*instrs)[pos]->isGEP() || !(*instrs)[pos + 1]->isStore())
        return;
    GEP *gep = (GEP *)(*instrs)[pos];
    Store *ST = (Store *)(*instrs)[pos + 1];
    Value *addr = gep->get_addr();
    if (is_a<Instrution>(addr))
        if (lson->is_BBinLoop((((Instrution *)addr)->get_parent())))
            return;
    if (!is_a<ConstantI32>(ST->get_src()))
        return;
    W1(lson);
}
void loopYYY::W3(Loop *loop)
{
    if (loop->get_BBs()->size() != 1)
        return;
    BasicBlock *BB = loop->get_header();
    if (BB->get_phinodes()->size() != 1)
        return;
    if (!loop->is_stepSet())
        return;
}

void loopYYY::FuncAnalysis(Function *func)
{
    Loop *Lroot = LA->get_LoopInfo()->find(func)->second;
    for (Loop *rt : *Lroot->get_lpsons())
    {
        switch (_getd(rt))
        {
        case 1:
            W1(rt);
            break;
        case 2:
            W2(rt);
            break;
        case 3:
            W3(rt);
            break;
        default:
            break;
        }
    }
}

void loopYYY::PassRun(Module *p_module)
{
    scev->PassRun(p_module);
    LA = scev->_Loop;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (!p_func->get_blocks()->empty())
            continue;
        if (p_func->get_name() == "memset")
            p_set = p_func;
    }
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncAnalysis(p_func);
        p_func->checkBBcond();
    }
}

loopYYY::loopYYY()
{
    scev = new SCEV;
}

loopYYY::~loopYYY()
{
    delete scev;
}