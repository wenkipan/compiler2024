#include <ir_opt/looptimes.hpp>
#include <ir_opt/IRCopy.hpp>
#include <ir_opt/LCSSA.hpp>
#include <algorithm>

std::set<BasicBlock *> looptimes::SubMap;

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
    nwPhi->addIncoming(used, nwfromBB);
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
                if (((Instrution *)_used)->get_parent() == loop->get_prev())
                    continue;
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

static inline void _dealUser(Value *val, Loop *loop, IRCopy &copyer)
{
    auto userlist = val->get_user_list();
    for (auto it = userlist->begin(); it != userlist->end(); ++it)
    {
        Instrution *_user = (Instrution *)(*it)->get_user();
        BasicBlock *_BB = _user->get_parent();
        if (loop->is_BBinLoop(_BB))
            continue;
        assert(is_a<PHINode>(_user));
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

static inline void _dealCond(BasicBlock *_exiting, BasicBlock *nextBB, Loop *loop, BasicBlock *nwexiting)
{
    assert(nwexiting->get_last_instrution()->isBranch());
    Branch *p_branch = ((Branch *)nwexiting->get_last_instrution());
    nwexiting->get_instrs()->pop_back();
    Value *p_cond = p_branch->get_cond();
    std::vector<Instrution *> *instrs = nwexiting->get_instrs();
    for (auto it = instrs->begin(); it != instrs->end(); ++it)
        if ((*it) == p_cond)
        {
            instrs->erase(it);
            break;
        }
    p_cond->drop();
    Constant *const1 = new ConstantI32(1), *const2 = new ConstantI32(0);
    _exiting->get_func()->value_pushBack(const1), _exiting->get_func()->value_pushBack(const2);
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
        p_cond = new Cmp(InstrutionEnum::IEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[1];
        (*nwexiting->get_user_list())[1] = newEdge;
    }
    else
    {
        p_cond = new Cmp(InstrutionEnum::INEQ, const1, const2, nwexiting);
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
    auto _V = nwexiting->get_func()->get_Values();
    assert(_V->back() == p_cond);
    _V->pop_back();
    _exiting->get_func()->value_pushBack(p_cond);
    new Edge(p_branch, p_cond);
    nwexiting->get_instrs()->push_back(p_branch);
    assert(p_branch->get_value_list()->size() == 1);
    BasicBlock *exit = *loop->get_exits()->begin();
    assert(loop->get_exits()->size() == 1);
    for (PHINode *Phi : *exit->get_phinodes())
        Phi->eraseIncoming(nwexiting);
}

static inline void _CutOutuser(Value *_val, Loop *loop, IRCopy &copyer, BasicBlock *PhiBB, BasicBlock *JudgeBB)
{
    auto Edges = _val->get_user_list();
    for (Edge *edge : *Edges)
    {
        Value *user = edge->get_user();
        BasicBlock *Pa = ((Instrution *)user)->get_parent();
        if (Pa == PhiBB || Pa == JudgeBB || loop->is_BBinLoop(Pa))
            continue;
        auto CuserList = copyer.get_mapval(_val)->get_user_list();
        Value *Cuser = copyer.get_mapval(user);
        for (auto it = CuserList->begin(); it != CuserList->end(); ++it)
            if ((*it)->get_user() == Cuser)
            {
                delete (*it);
                CuserList->erase(it);
                break;
            }
    }
}

static inline void _SetOutuser(Value *_val, Loop *loop, IRCopy &copyer, BasicBlock *PhiBB, BasicBlock *JudgeBB)
{
    std::vector<Edge *> *edges = _val->get_user_list();
    auto it = edges->begin();
    std::set<Edge *> delE;
    PHINode *newPhi = nullptr;
    for (; it != edges->end();)
    {
        Instrution *_user = (Instrution *)(*it)->get_user();
        BasicBlock *Pa = _user->get_parent();
        if (!loop->is_BBinLoop(Pa) && Pa != JudgeBB && Pa != PhiBB && delE.find(*it) == delE.end())
        {

            if (newPhi == nullptr)
            {
                newPhi = new PHINode(PhiBB, _val->get_type()->get_type(), true);
                newPhi->addIncoming(_val, JudgeBB);
                newPhi->addIncoming(copyer.get_mapval(_val), copyer.get_mapbb(*loop->get_exitings()->begin()));
                PhiBB->Insert_Phi(newPhi);
                it = edges->begin();
                continue;
            }
            std::vector<Edge *> *Cedges = copyer.get_mapval(_val)->get_user_list();
            auto Cuser = copyer.get_mapval((*it)->get_user());
            for (auto edge = Cedges->begin(); edge != Cedges->end(); ++edge)
            {
                if ((*edge)->get_user() == Cuser)
                {
                    delete *edge;
                    Cedges->erase(edge);
                    break;
                }
            }
            (*it)->set_val(newPhi);
            if (_user->isPHINode())
            {
                PHINode *userPhi = (PHINode *)_user;
                BasicBlock *_fromBB = userPhi->get_edge_income_block(*it);
                userPhi->get_valueMap()->erase(_fromBB);
                if (_fromBB == (*loop->get_exitings()->begin()))
                    _fromBB = PhiBB;
                userPhi->get_valueMap()->insert({_fromBB, (*it)});
            }
            delE.insert(*it);
        }
        else
            ++it;
    }
    if (delE.empty())
        return;
    auto valsEdges = _val->get_user_list();
    for (Edge *deledge : delE)
        valsEdges->erase(std::remove(valsEdges->begin(), valsEdges->end(), deledge), valsEdges->end());
}

static inline void _dealLast(Loop *loop, IRCopy &Fcopyer, IRCopy &Lcopyer, BasicBlock *_exit)
{
    BasicBlock *_exiting = *loop->get_exitings()->begin();
    BasicBlock *_Lexiting = Lcopyer.get_mapbb(_exiting);
    BasicBlock *p_true = nullptr, *p_false = nullptr;
    if (loop->is_BBinLoop((BasicBlock *)((*_exiting->get_user_list()->begin())->get_val())))
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
    loop->get_lpStep()->print();
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

BasicBlock *looptimes::Unroll(Loop *loop, const int times, Function *p_func)
{
    Function *func[128];
    IRCopy copyer[128];
    for (int i = 0; i < times; ++i)
        func[i] = copyer[i].copy_func(p_func);

    BasicBlock *_Judge = new BasicBlock(p_func);
    BasicBlock *_PhiBB = new BasicBlock(p_func);
    p_func->get_blocks()->push_back(_Judge), p_func->get_blocks()->push_back(_PhiBB);
    // set JBB branch
    Cmp *_LPCmp = (Cmp *)loop->get_lpCmp();
    Cmp *_JudgeCmp = new Cmp(_LPCmp->get_Instrtype(), _LPCmp->get_src1(), _LPCmp->get_src2(), _Judge);
    BasicBlock *_TBB = _PhiBB, *_FBB = copyer[0].get_mapbb(loop->get_header());
    BasicBlock *lpexiting = *loop->get_exitings()->begin();
    BasicBlock *lplatch = *loop->get_latchs()->begin();
    if (loop->is_BBinLoop((BasicBlock *)((*lpexiting->get_user_list()->begin())->get_user())))
        std::swap(_TBB, _FBB);
    _Judge->Set_branch(_JudgeCmp, _TBB, _FBB);

    for (int i = 0; i < times; ++i)
    {
        std::set<Value *> _addValues;
        std::vector<BasicBlock *> _addBBs;
        auto _BBs = loop->get_BBs();
        if (i != times - 1)
        {
            for (BasicBlock *_BB : *_BBs)
            {
                _addBBs.emplace_back(copyer[i].get_mapbb(_BB));
                for (PHINode *_Phi : *_BB->get_phinodes())
                {
                    _addValues.insert(copyer[i].get_mapval(_Phi));
                    _dealVal(_Phi, loop, _addValues, copyer[i]);
                    _CutOutuser(_Phi, loop, copyer[i], _PhiBB, _Judge);
                }
                std::vector<Instrution *> *instrs = _BB->get_instrutions();
                for (Instrution *p_instr : *instrs)
                {
                    _addValues.insert(copyer[i].get_mapval(p_instr));
                    copyer[i].get_mapval(p_instr)->print();
                    _dealVal(p_instr, loop, _addValues, copyer[i]);
                    _CutOutuser(p_instr, loop, copyer[i], _PhiBB, _Judge);
                }
            }
            BasicBlock *nwexiting = copyer[i].get_mapbb(lpexiting);
            BasicBlock *nextHeader = copyer[i + 1].get_mapbb(loop->get_header());
            BasicBlock *nextprev = copyer[i + 1].get_mapbb(loop->get_prev());
            _dealCond(lpexiting, nextHeader, loop, nwexiting);
            for (PHINode *Phi : *loop->get_header()->get_phinodes())
            {
                PHINode *nextPhi = ((PHINode *)copyer[i + 1].get_mapval(Phi));
                nextPhi->eraseIncoming(nextprev);
                nextPhi->addIncoming(copyer[i].get_mapval(Phi->get_valueMap()->find(lplatch)->second->get_val()), nwexiting);
            }
            for (auto it = nextHeader->get_value_list()->begin(); it != nextHeader->get_value_list()->end(); ++it)
                if ((*it)->get_val() == nextprev)
                {
                    nextHeader->get_value_list()->erase(it);
                    break;
                }
        }
        else
        {
            for (BasicBlock *_BB : *_BBs)
            {
                _addBBs.emplace_back(copyer[i].get_mapbb(_BB));
                for (PHINode *_Phi : *_BB->get_phinodes())
                {
                    _addValues.insert(copyer[i].get_mapval(_Phi));
                    _dealVal(_Phi, loop, _addValues, copyer[i]);
                    _SetOutuser(_Phi, loop, copyer[i], _PhiBB, _Judge);
                }
                std::vector<Instrution *> *instrs = _BB->get_instrutions();
                for (Instrution *p_instr : *instrs)
                {
                    _addValues.insert(copyer[i].get_mapval(p_instr));
                    _dealVal(p_instr, loop, _addValues, copyer[i]);
                    _SetOutuser(p_instr, loop, copyer[i], _PhiBB, _Judge);
                }
            }
            _dealLast(loop, copyer[0], copyer[i], _PhiBB);
        }

        auto nwblocks = func[i]->get_blocks();
        for (BasicBlock *addBB : _addBBs)
        {
            nwblocks->erase(std::remove(nwblocks->begin(), nwblocks->end(), addBB), nwblocks->end());
            p_func->block_pushBack(addBB);
            addBB->Set_parent(p_func);
        }
        auto nwValues = func[i]->get_Values();
        for (Value *addval : _addValues)
        {
            nwValues->erase(std::remove(nwValues->begin(), nwValues->end(), addval), nwValues->end());
            p_func->value_pushBack(addval);
        }
        puts("");
    }
    _AddLoopIf(loop, times, _SCEV, _Judge);
    for (PHINode *Phi : *loop->get_header()->get_phinodes())
    {
        PHINode *_nextPhi = ((PHINode *)copyer[0].get_mapval(Phi));
        _nextPhi->eraseIncoming(copyer[0].get_mapbb(loop->get_prev()));
        PHINode *NwPhi = new PHINode(_Judge, _nextPhi->get_type()->get_type(), true);
        // p_func->value_pushBack(NwPhi);
        _Judge->Insert_Phi(NwPhi);
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
        NwPhi->addIncoming(Phi->get_valueMap()->find(loop->get_prev())->second->get_val(), loop->get_prev());
        NwPhi->addIncoming(Phi->get_valueMap()->find(lplatch)->second->get_val(), (*loop->get_exitings()->begin()));
        _nextPhi->addIncoming(NwPhi, _Judge);
    }
    BasicBlock *header1 = copyer[0].get_mapbb(loop->get_header());
    for (auto it = header1->get_value_list()->begin(); it != header1->get_value_list()->end(); ++it)
    {
        if ((*it)->get_val() == copyer[0].get_mapbb(loop->get_prev()))
        {
            header1->get_value_list()->erase(it);
            break;
        }
    }
    BasicBlock *_exit = *loop->get_exits()->begin();
    for (auto it = _exit->get_value_list()->begin(); it != _exit->get_value_list()->end(); ++it)
        if (((*it)->get_val() == lpexiting))
        {
            (*it)->set_user(_Judge);
            _exit->get_value_list()->erase(it);
            break;
        }
    _PhiBB->Set_jmp(_exit);
    for (PHINode *Phi : *_PhiBB->get_phinodes())
    {
        for (auto i : *Phi->get_valueMap())
        {
            if (i.first != _Judge)
                continue;
            if (is_a<Constant>((*i.second).get_val()))
                continue;
            Instrution *p_instr = (Instrution *)(*i.second).get_val();
            if (!loop->is_BBinLoop(p_instr->get_parent()))
                continue;
            PHINode *newPhi = new PHINode(_Judge, p_instr->get_type()->get_type(), true);
            _Judge->Insert_Phi(newPhi);
            Constant *_const = nullptr;
            if (p_instr->get_type()->get_type() == TypeEnum::I32)
                _const = new ConstantI32(0);
            else if (p_instr->get_type()->get_type() == TypeEnum::F32)
                _const = new ConstantF32(0.0);
            else
                assert(0);
            p_func->value_pushBack(_const);
            newPhi->addIncoming(_const, loop->get_prev());
            auto p_edges = p_instr->get_user_list();
            for (auto j = p_edges->begin(); j != p_edges->end(); ++j)
            {
                if (i.second == *j)
                {
                    p_edges->erase(j);
                    break;
                }
            }
            newPhi->addIncoming(i.second->get_val(), lpexiting);
            i.second->set_val(newPhi);
            break;
        }
    }
    for (int i = 0; i < times; ++i)
        delete func[i];
    return loop->get_header();
}

static inline void _dealExit(BasicBlock *lpexiting, Loop *loop, IRCopy &copyer)
{
    BasicBlock *nwexiting = copyer.get_mapbb(lpexiting);
    Branch *p_branch = ((Branch *)nwexiting->get_last_instrution());
    nwexiting->get_instrs()->pop_back();
    Value *p_cond = p_branch->get_cond();
    assert(p_cond == nwexiting->get_last_instrution());
    nwexiting->Ins_popBack();
    p_cond->drop();
    Constant *const1 = new ConstantI32(1), *const2 = new ConstantI32(0);
    lpexiting->get_func()->value_pushBack(const1), lpexiting->get_func()->value_pushBack(const2);
    BasicBlock *exitBB = *loop->get_exits()->begin();
    Edge *newEdge = new Edge(exitBB, nwexiting);
    nwexiting->get_user_list()->pop_back();
    Edge *delEdge = nullptr;
    if (loop->is_BBinLoop(((Branch *)lpexiting->get_last_instrution())->get_trueBB()))
    {
        p_cond = new Cmp(InstrutionEnum::IEQ, const1, const2, nwexiting);
        delEdge = (*nwexiting->get_user_list())[1];
        (*nwexiting->get_user_list())[1] = newEdge;
    }
    else
    {
        p_cond = new Cmp(InstrutionEnum::INEQ, const1, const2, nwexiting);
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
    auto _V = nwexiting->get_func()->get_Values();
    assert(_V->back() == p_cond);
    _V->pop_back();
    lpexiting->get_func()->value_pushBack(p_cond);
    new Edge(p_branch, p_cond);
    nwexiting->get_instrs()->push_back(p_branch);
    assert(p_branch->get_value_list()->size() == 1);
}

void looptimes::FullUnroll(Loop *loop, int times, Function *p_func)
{
    times = times - 2;
    Function *func[128];
    IRCopy copyer[128];
    for (int i = 0; i <= times; ++i)
        func[i] = copyer[i].copy_func(p_func);
    for (int i = 0; i <= times; ++i)
    {
        std::set<Value *> _addValues;
        std::vector<BasicBlock *> _addBBs;
        auto _BBs = loop->get_BBs();
        for (BasicBlock *_BB : *_BBs)
        {
            _addBBs.emplace_back(copyer[i].get_mapbb(_BB));
            for (PHINode *_Phi : *_BB->get_phinodes())
            {
                _addValues.insert(copyer[i].get_mapval(_Phi));
                _dealVal(_Phi, loop, _addValues, copyer[i]);
                _dealUser(_Phi, loop, copyer[i]);
            }
            std::vector<Instrution *> *instrs = _BB->get_instrutions();
            for (Instrution *p_instr : *instrs)
            {
                _addValues.insert(copyer[i].get_mapval(p_instr));
                copyer[i].get_mapval(p_instr)->print();
                _dealVal(p_instr, loop, _addValues, copyer[i]);
                _dealUser(p_instr, loop, copyer[i]);
            }
        }
        auto nwblocks = func[i]->get_blocks();
        for (BasicBlock *addBB : _addBBs)
        {
            nwblocks->erase(std::remove(nwblocks->begin(), nwblocks->end(), addBB), nwblocks->end());
            p_func->block_pushBack(addBB);
            addBB->Set_parent(p_func);
        }
        auto nwValues = func[i]->get_Values();
        for (Value *addval : _addValues)
        {
            nwValues->erase(std::remove(nwValues->begin(), nwValues->end(), addval), nwValues->end());
            p_func->value_pushBack(addval);
        }
    }
    BasicBlock *lpexiting = *loop->get_exitings()->begin();
    BasicBlock *lplatch = *loop->get_latchs()->begin();
    for (int i = 0; i < times; ++i)
    {
        BasicBlock *nwexiting = copyer[i].get_mapbb(lpexiting);
        BasicBlock *nextHeader = copyer[i + 1].get_mapbb(loop->get_header());
        BasicBlock *nextPrev = copyer[i + 1].get_mapbb(loop->get_prev());
        _dealCond(lpexiting, nextHeader, loop, copyer[i].get_mapbb(lpexiting));
        for (PHINode *Phi : *loop->get_header()->get_phinodes())
        {
            PHINode *nextPhi = ((PHINode *)copyer[i + 1].get_mapval(Phi));
            nextPhi->eraseIncoming(nextPrev);
            nextPhi->addIncoming(copyer[i].get_mapval(Phi->get_valueMap()->find(lplatch)->second->get_val()), nwexiting);
        }
        for (auto it = nextHeader->get_value_list()->begin(); it != nextHeader->get_value_list()->end(); ++it)
            if ((*it)->get_val() == nextPrev)
            {
                nextHeader->get_value_list()->erase(it);
                break;
            }
    }
    _dealExit(lpexiting, loop, copyer[times]);
    _dealCond(lpexiting, copyer[0].get_mapbb(loop->get_header()), loop, lpexiting);
    BasicBlock *nextHeader = copyer[0].get_mapbb(loop->get_header());
    BasicBlock *nextPrev = copyer[0].get_mapbb(loop->get_prev());
    for (PHINode *Phi : *loop->get_header()->get_phinodes())
    {
        PHINode *nextPhi = ((PHINode *)copyer[0].get_mapval(Phi));
        nextPhi->eraseIncoming(nextPrev);
        nextPhi->addIncoming(Phi->get_valueMap()->find(lplatch)->second->get_val(), lpexiting);
    }

    for (auto it = nextHeader->get_value_list()->begin(); it != nextHeader->get_value_list()->end(); ++it)
        if ((*it)->get_val() == nextPrev)
        {
            nextHeader->get_value_list()->erase(it);
            break;
        }

    for (int i = 0; i <= times; ++i)
        delete func[i];
}

static bool _checkLoop(Loop *loop)
{
    bool re = true;
    for (Loop *son : *loop->get_lpsons())
    {
        re = re & _checkLoop(son);
    }
    if (loop->get_exitings()->size() == 1 && loop->get_latchs()->size() == 1 && (*loop->get_exitings()->begin()) == (*loop->get_latchs()->begin()))
        return re;
    else
        return false;
}

static inline int _checkFull(Loop *loop, SCEV *scev)
{

    int re = -1;
    if (!loop->is_stepSet())
        return -1;
    SCEVEXP *exp = scev->find_exp(loop->get_lpStep());
    if (exp == nullptr)
        return -1;
    auto dims = exp->get_dims();
    if (!(*dims)[2].empty())
        return -1;
    int StepBegin = 0x3f3f3f3f, StepNum = 0x3f3f3f3f, StepEnd = 0x3f3f3f3f;
    if ((*dims)[1].size() != 1 || (*dims)[0].size() != 1)
        return -1;
    auto it = (*dims)[1].begin();
    if (is_a<ConstantI32>(it->first))
    {
        assert(it->second != SCEVType::MUL);
        StepNum = *((ConstantI32 *)it->first)->get_i32().begin();
        if (it->second == SCEVType::SUB)
            StepNum *= -1;
    }
    else
        return -1;

    it = (*dims)[0].begin();
    if (is_a<PHINode>(it->first))
    {

        auto valMap = ((PHINode *)it->first)->get_valueMap();
        if (valMap->size() != 1)
            return -1;
        auto It = valMap->begin()->second->get_val();
        ConstantI32 *Cnum = dynamic_cast<ConstantI32 *>(It);
        if (Cnum != nullptr)
        {
            StepBegin = *Cnum->get_i32().begin();
            assert(it->second == SCEVType::ADD);
        }
        else
            return -1;
    }
    else
        return -1;

    if (is_a<ConstantI32>(loop->get_lpEnd()))
        StepEnd = *((ConstantI32 *)loop->get_lpEnd())->get_i32().begin();
    else
        return -1;

    int _type = loop->get_cmpType();
    switch (_type)
    {
    case 1:
        StepEnd--;
    case 0:
        re = (StepEnd - StepBegin + StepNum + 1) / StepNum;
        break;
    case 3:
        StepEnd++;
    case 2:
        re = (StepEnd - StepBegin + StepNum - 1) / StepNum;
        break;
    default:
        assert(0);
        break;
    }
    printf("re = %d\n", re);
    return re ? re : -1;
}

static inline bool _Strategy(Loop *loop, int times, int &codesize)
{
    if (times > 100 || times == 1)
        return false;
    int cnt = 0;
    for (BasicBlock *BB : *loop->get_BBs())
        cnt += BB->get_phinodes()->size() + BB->get_instrs()->size();
    cnt *= times;

    if (cnt > 2000)
        return false;
    else if (cnt > 1000)
    {
        if (codesize < 1000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (cnt > 500)
    {
        if (codesize < 2000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (cnt > 200)
    {
        if (codesize < 5000)
        {
            codesize += cnt;
            return true;
        }
    }
    else if (codesize <= 8000)
    {
        codesize += cnt;
        return true;
    }
    return false;
}

bool looptimes::Finder(Loop *loop, Function *func)
{
    const int Unrolltimes = 8;
    bool flag = false;
    for (Loop *son : *loop->get_lpsons())
    {
        flag = flag | Finder(son, func);
    }
    if (flag == true)
        return true;
    if (loop->get_exitings()->size() != 1 || loop->get_latchs()->size() != 1)
        return false;
    if (*loop->get_exitings()->begin() != *loop->get_latchs()->begin() || !loop->is_stepSet())
        return false;
    if (_checkLoop(loop))
    {
        int times = _checkFull(loop, _SCEV);
        if (times != -1 && _Strategy(loop, times, codesize))
        {
            FullUnroll(loop, times, func);
            return true;
        }
        else if (times != 1 && _Strategy(loop, Unrolltimes, codesize) && SubMap.find(loop->get_header()) == SubMap.end())
        {
            BasicBlock *header = Unroll(loop, Unrolltimes, func);
            SubMap.insert(header);
            return true;
        }
    }
    return false;
}

void looptimes::FuncDealer(Function *p_func)
{
    Loop *loop = _Loop->get_LoopInfo()->find(p_func)->second;
    LCSSA worker(p_func, _Loop);
    worker.run();
    bool flag = false;
    for (Loop *son : *loop->get_lpsons())
    {
        if (flag)
            break;
        flag = flag | Finder(son, p_func);
    }
}

void looptimes::PassRun(Module *p_module)
{
    puts("\n          Unrolling        \n");
    _SCEV->PassRun(p_module);
    _Loop = _SCEV->_Loop;
    codesize = 0;
    puts("UUUUbegin");
    for (Function *p_func : *p_module->get_funcs())
        codesize += p_func->get_Values()->size() - 50;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        FuncDealer(p_func);
    }
}

looptimes::looptimes()
{
    _SCEV = new SCEV();
}

looptimes::~looptimes()
{
    delete _SCEV;
}