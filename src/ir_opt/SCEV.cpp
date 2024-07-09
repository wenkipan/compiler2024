#include <ir_opt/SCEV.hpp>

std::unordered_map<Value *, SCEVEXP *> *SCEVEXP::SCEVMAP = nullptr;

Value *SCEVEXP::_getExpVal(SCEVEXP *val, int i, BasicBlock *_BB)
{
    Value *re = nullptr;
    int flag = 0;
    auto dims = val->get_dims();
    assert(!(*dims)[i].empty());
    if ((*dims)[i][0].first->get_type()->get_type() == TypeEnum::I32)
        re = new ConstantI32(0), flag = 0;
    else if ((*dims)[i][0].first->get_type()->get_type() == TypeEnum::F32)
        re = new ConstantF32(0.0), flag = 5;
    else
        assert(0);
    _BB->get_func()->value_pushBack(re);
    if (i >= 3)
        assert(0);
    Instrution *p_instr = nullptr;
    int nw = -1;
    std::vector<std::pair<Value *, SCEVType>> *dim;
    for (auto &it : (*dims)[i])
    {
        switch (it.second)
        {
        case SCEVType::ADD:
            p_instr = new Binary((InstrutionEnum)(24 + flag), re, it.first, _BB);
            break;
        case SCEVType::SUB:
            p_instr = new Binary((InstrutionEnum)(25 + flag), re, it.first, _BB);
            break;
        case SCEVType::MUL:
            p_instr = new Binary((InstrutionEnum)(26 + flag), re, it.first, _BB);
            break;
        case SCEVType::EXPADD0:
            nw = nw == -1 ? 0 : nw;
        case SCEVType::EXPADD1:
            nw = nw == -1 ? 1 : nw;
        case SCEVType::EXPADD2:
            nw = nw == -1 ? 2 : nw;
            dim = &(*SCEVMAP->find(it.first)->second->get_dims())[nw];
            if (dim->empty())
                break;
            p_instr = new Binary((InstrutionEnum)(24 + flag), re, _getExpVal(SCEVMAP->find(it.first)->second, nw, _BB), _BB);
            break;
        case SCEVType::EXPSUB0:
            nw = nw == -1 ? 0 : nw;
        case SCEVType::EXPSUB1:
            nw = nw == -1 ? 1 : nw;
        case SCEVType::EXPSUB2:
            nw = nw == -1 ? 2 : nw;
            dim = &(*SCEVMAP->find(it.first)->second->get_dims())[nw];
            if (dim->empty())
                break;
            p_instr = new Binary((InstrutionEnum)(25 + flag), re, _getExpVal(SCEVMAP->find(it.first)->second, nw, _BB), _BB);
            break;
        default:
            break;
        }
        re = (Value *)p_instr;
    }
    return re;
}

Value *SCEVEXP::get_scr(int i, BasicBlock *_BB)
{
    Value *re = nullptr;
    int flag = 0;
    assert(!(*dims)[i].empty());
    if ((*dims)[i][0].first->get_type()->get_type() == TypeEnum::I32)
        re = new ConstantI32(0), flag = 0;
    else if ((*dims)[i][0].first->get_type()->get_type() == TypeEnum::F32)
        re = new ConstantF32(0.0), flag = 5;
    else
        assert(0);
    _BB->get_func()->value_pushBack(re);
    Instrution *p_instr = nullptr;
    int nw = -1;
    std::vector<std::pair<Value *, SCEVType>> *dim;
    for (auto &it : (*dims)[i])
    {
        switch (it.second)
        {
        case SCEVType::ADD:
            p_instr = new Binary((InstrutionEnum)(24 + flag), re, it.first, _BB);
            break;
        case SCEVType::SUB:
            p_instr = new Binary((InstrutionEnum)(25 + flag), re, it.first, _BB);
            break;
        case SCEVType::MUL:
            p_instr = new Binary((InstrutionEnum)(26 + flag), re, it.first, _BB);
            break;
        case SCEVType::EXPADD0:
            nw = nw == -1 ? 0 : nw;
        case SCEVType::EXPADD1:
            nw = nw == -1 ? 1 : nw;
        case SCEVType::EXPADD2:
            nw = nw == -1 ? 2 : nw;
            dim = &(*SCEVMAP->find(it.first)->second->get_dims())[nw];
            if (dim->empty())
                break;
            p_instr = new Binary((InstrutionEnum)(24 + flag), re, _getExpVal(SCEVMAP->find(it.first)->second, nw, _BB), _BB);
            break;
        case SCEVType::EXPSUB0:
            nw = nw == -1 ? 0 : nw;
        case SCEVType::EXPSUB1:
            nw = nw == -1 ? 1 : nw;
        case SCEVType::EXPSUB2:
            nw = nw == -1 ? 2 : nw;
            dim = &(*SCEVMAP->find(it.first)->second->get_dims())[nw];
            if (dim->empty())
                break;
            p_instr = new Binary((InstrutionEnum)(25 + flag), re, _getExpVal(SCEVMAP->find(it.first)->second, nw, _BB), _BB);
            break;
        default:
            break;
        }
        re = (Value *)p_instr;
    }

    return re;
}

static bool inline _isInVar(Value *_val, Loop *loop)
{
    if (is_a<Constant>(_val) || loop->get_BBs()->find(((Instrution *)_val)->get_parent()) == loop->get_BBs()->end())
        return true;
    return false;
}

void SCEV::LoopSetStep(Loop *loop)
{
    for (Loop *_son : (*loop->get_lpsons()))
        LoopSetStep(_son);
    if (!loop->get_lpDepth())
        return;
    if (!loop->is_simple())
        return;
    BasicBlock *_latch = (*loop->get_latchs()->begin());
    BasicBlock *_exiting = (*loop->get_exitings()->begin());
    assert(_latch && _exiting);
    Branch *p_branch = nullptr;
    assert((p_branch = dynamic_cast<Branch *>(_exiting->get_last_instrution())));
    Cmp *p_cmp = nullptr;
    assert((p_cmp = dynamic_cast<Cmp *>(p_branch->get_cond())));
    int cmpType = 0;

    switch (p_cmp->get_Instype())
    {
    case InstrutionEnum::IGT:
    case InstrutionEnum::FGT:
        cmpType = 0;
        break;
    case InstrutionEnum::IGE:
    case InstrutionEnum::FGE:
        cmpType = 1;
        break;
    case InstrutionEnum::ILT:
    case InstrutionEnum::FLT:
        cmpType = 2;
        break;
    case InstrutionEnum::ILE:
    case InstrutionEnum::FLE:
        cmpType = 3;
        break;
    default:
        return;
    }
    SCEVEXP *p_exp = nullptr;
    if (_isInVar(p_cmp->get_src1(), loop) && (p_exp = find_exp(p_cmp->get_src2())) != nullptr)
    {

        cmpType = (cmpType + 2) % 4;
        loop->set_lpStep(p_exp->get_ToPhi());
        loop->set_lpEnd(p_cmp->get_src1());
    }
    else if ((p_exp = find_exp(p_cmp->get_src1())) != nullptr && _isInVar(p_cmp->get_src2(), loop))
    {
        loop->set_lpStep(p_exp->get_ToPhi());
        loop->set_lpEnd(p_cmp->get_src2());
    }
    else
        return;
    p_exp = SCEVMAP->find(p_exp->get_ToPhi())->second;
    if (!(*p_exp->get_dims())[2].empty() || !(*p_exp->get_dims())[3].empty() || !(*p_exp->get_dims())[4].empty())
        return;
    loop->set_ifStep(true);
    loop->set_lpCmp(p_cmp);
    loop->set_cmpType(cmpType);
    return;
    putchar('\n');
    p_cmp->print();
    printf("   ");
    find_exp(loop->get_lpStep())->print();
    putchar('\n');
}

void SCEV::SetStep()
{
    printf("loopStep\n");
    for (auto it : (*_Loop->get_LoopInfo()))
        LoopSetStep(it.second);
    printf("loopStep Finish\n");
}

void SCEVEXP::print()
{
    int cnt = 0;
    for (auto i : *dims)
    {

        if (i.empty())
            break;
        if (i.begin()->second > SCEVType::MUL)
        {
            i.clear();
            break;
        }
        if (cnt)
            printf(", ");
        int cnt1 = 0;
        int nw = -1;
        for (auto j : i)
        {
            if (cnt1)
                printf(" + ");
            switch (j.second)
            {
            case SCEVType::EXPADD0:
                nw = nw == -1 ? 0 : nw;
            case SCEVType::EXPADD1:
                nw = nw == -1 ? 1 : nw;
            case SCEVType::EXPADD2:
                nw = nw == -1 ? 2 : nw;
                break;
            case SCEVType::MUL:
                putchar('*');
                break;
            case SCEVType::SUB:
            case SCEVType::EXPSUB0:
                nw = nw == -1 ? 0 : nw;
            case SCEVType::EXPSUB1:
                nw = nw == -1 ? 1 : nw;
            case SCEVType::EXPSUB2:
                nw = nw == -1 ? 2 : nw;
                putchar('-');
                break;
            default:
                break;
            }
            if (j.second > SCEVType::MUL)
                printf("(dim[%d])", nw);

            j.first->print_ID();
            cnt1++;
        }
        ++cnt;
    }
    if (_mod != nullptr)
    {
        printf("; mod ");
        _mod->print_ID();
    }
}

void SCEV::print()
{
    printf("Map size = %zu\n", SCEVMAP->size());
    for (auto it : *SCEVMAP)
    {
        it.first->print_ID();
        printf(" = ");
        it.second->print();
        putchar('\n');
    }
}

SCEVEXP *SCEV::find_exp(Value *_val)
{
    auto it = SCEVMAP->find(_val);
    if (it == SCEVMAP->end())
        return nullptr;
    return it->second;
}

void _dfs(Value *_val, Loop *loop, SCEV *_this, std::vector<std::vector<std::pair<Value *, SCEVType>>> &dims, int &flags)
{
    if (flags >= 2000)
        return;
    assert(is_a<Instrution>(_val));
    Instrution *p_instr = (Instrution *)_val;
    Binary *_binary = nullptr;
    bool flag;
    SCEVEXP *p_exp = nullptr;
    switch (p_instr->get_Instrtype())
    {
    case InstrutionEnum::PHINode:
        if (loop->get_header() == p_instr->get_parent())
            flags += 1000;
        else
            flags += 3000;
        break;
    case InstrutionEnum::IADD:
    case InstrutionEnum::ISUB:
    case InstrutionEnum::FADD:
    case InstrutionEnum::FSUB:
        if (p_instr->get_Instrtype() == InstrutionEnum::ISUB || p_instr->get_Instrtype() == InstrutionEnum::FSUB)
            flag = true;
        else
            flag = false;
        _binary = (Binary *)p_instr;
        if (_isInVar(_binary->get_src1(), loop))
            dims[1].emplace_back(std::make_pair(_binary->get_src1(), SCEVType::ADD));
        else if ((p_exp = _this->find_exp(_binary->get_src1())) != nullptr)
        {
            if (p_exp->is_mod())
                flags += 3000;
            for (int i = 0; i < 4; ++i)
            {
                dims[i + 1].insert(dims[i + 1].end(), (*p_exp->get_dims())[i].begin(), (*p_exp->get_dims())[i].end());
            }
            if (!(*p_exp->get_dims())[4].empty())
                flags += 3000;
        }
        else
            _dfs(_binary->get_src1(), loop, _this, dims, flags);

        if (_isInVar(_binary->get_src2(), loop))
            dims[1].emplace_back(std::make_pair(_binary->get_src2(), flag ? SCEVType::SUB : SCEVType::ADD));
        else if ((p_exp = _this->find_exp(_binary->get_src2())) != nullptr)
        {
            if (p_exp->is_mod())
                flags += 3000;
            for (int i = 0; i < 4; ++i)
            {
                if (!flag)
                {
                    dims[i + 1].insert(dims[i + 1].end(), (*p_exp->get_dims())[i].begin(), (*p_exp->get_dims())[i].end());
                    continue;
                }
                for (auto it : (*p_exp->get_dims())[i])
                {
                    if (it.second == SCEVType::ADD)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::SUB));
                    else if (it.second == SCEVType::SUB)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::ADD));
                    else if (it.second == SCEVType::EXPADD0)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB0));
                    else if (it.second == SCEVType::EXPSUB0)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPADD0));
                    else if (it.second == SCEVType::EXPADD1)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB1));
                    else if (it.second == SCEVType::EXPSUB1)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPADD1));
                    else if (it.second == SCEVType::EXPADD2)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB2));
                    else if (it.second == SCEVType::EXPSUB2)
                        dims[i + 1].emplace_back(std::make_pair(it.first, SCEVType::EXPADD2));
                    else
                        flags += 3000;
                }
            }
            if (!(*p_exp->get_dims())[4].empty())
                flags += 3000;
        }
        else
            _dfs(_binary->get_src2(), loop, _this, dims, flags);
        break;
    case InstrutionEnum::MINUS:
        //?
        flags += 3000;
        break;
    default:
        flags += 3000;
        break;
    }
}

static inline bool PHIAnalysis(Loop *loop, SCEV *_this)
{
    bool re = false;
    BasicBlock *headBB = loop->get_header();
    assert(loop->get_latchs()->size() == 1);
    BasicBlock *latch = *(*loop->get_latchs()).begin();
    assert(loop->get_prev());
    BasicBlock *prev = loop->get_prev();
    for (PHINode *_phi : (*headBB->get_phinodes()))
    {
        Value *_val = _phi->get_valueMap()->find(latch)->second->get_val();
        if (_this->find_exp(_val) != nullptr)
            continue;
        if (!is_a<Instrution>(_val))
            continue;
        assert(is_a<Instrution>(_val));
        Instrution *p_instr = (Instrution *)_val;
        if (loop->get_nwBBs()->find(p_instr->get_parent()) == loop->get_nwBBs()->end())
            continue;
        if (!is_a<Binary>(_val))
            continue;
        Binary *_binary = (Binary *)p_instr;
        SCEVEXP *_scev;
        int bits = 0;
        SCEVEXP *tmp = nullptr;
        Value *_mod = nullptr;
        std::vector<std::vector<std::pair<Value *, SCEVType>>> dims(5, std::vector<std::pair<Value *, SCEVType>>());
        for (int i = 0; i < 5; ++i)
            dims[i].clear();
        dims[0].push_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
        switch (p_instr->get_Instrtype())
        {
        case InstrutionEnum::IMOD:
            // p_instr->print();
            if (_isInVar(_binary->get_src2(), loop) && _this->find_exp(_binary->get_src1()) == nullptr)
            {
                _mod = _binary->get_src2();
                if (!is_a<Binary>(_binary->get_src1()))
                    break;
                _binary = (Binary *)_binary->get_src1();
            }
        case InstrutionEnum::IADD:
        case InstrutionEnum::ISUB:
        case InstrutionEnum::FADD:
        case InstrutionEnum::FSUB:
            _binary->print();
            puts("ssssssssssss");
            _dfs(_binary, loop, _this, dims, bits);
            if (!bits || bits >= 2000)
                break;
            _scev = new SCEVEXP();
            tmp = _scev;
            if (_mod != nullptr)
                _scev->set_mod(_mod);
            for (int i = 0; i < 4; ++i)
                (*_scev->get_dims())[i] = dims[i];
            re = true;
            _this->get_map()->insert({_phi, _scev});
            _scev = new SCEVEXP();
            if (_mod != nullptr)
                _scev->set_mod(_mod);
            for (int i = 0; i < 4; ++i)
                (*_scev->get_dims())[i] = dims[i];
            for (int i = 0; i < 3; ++i)
            {
                if (!(*tmp->get_dims())[i + 1].empty())
                {
                    if (i == 0)
                        (*_scev->get_dims())[i].push_back(std::make_pair(_phi, SCEVType::EXPADD1));
                    else if (i == 1)
                        (*_scev->get_dims())[i].push_back(std::make_pair(_phi, SCEVType::EXPADD2));
                    else
                        assert(0);
                }
            }
            _scev->set_ToPhi(_phi);
            _this->get_map()->insert({p_instr, _scev});
            break;
        default:
            break;
        }
    }
    return re;
}
/*
static inline bool _PHIAnalysis(Loop *loop, SCEV *_this)
{
    bool re = false;
    BasicBlock *headBB = loop->get_header();
    assert(loop->get_latchs()->size() == 1);
    BasicBlock *latch = *(*loop->get_latchs()).begin();
    assert(loop->get_prev());
    BasicBlock *prev = loop->get_prev();
    SCEVType _flag;
    SCEVEXP *p_exp;
    for (PHINode *_phi : (*headBB->get_phinodes()))
    {
        Value *_val = _phi->get_valueMap()->find(latch)->second->get_val();
        if (_this->find_exp(_val) != nullptr)
            continue;
        assert(is_a<Instrution>(_val));
        Instrution *p_instr = (Instrution *)_val;
        if (loop->get_nwBBs()->find(p_instr->get_parent()) == loop->get_nwBBs()->end())
            continue;
        if (!is_a<Binary>(_val))
            continue;
        Binary *_binary = (Binary *)p_instr;
        SCEVEXP *_scev = nullptr;
        std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = nullptr;
        switch (p_instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
        case InstrutionEnum::ISUB:
        case InstrutionEnum::FADD:
        case InstrutionEnum::FSUB:
            if (p_instr->get_Instrtype() == InstrutionEnum::ISUB || p_instr->get_Instrtype() == InstrutionEnum::FSUB)
                _flag = SCEVType::SUB;
            else
                _flag = SCEVType::ADD;
            if (_binary->get_src1() == _phi && _isInVar(_binary->get_src2(), loop))
            {
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[1].emplace_back(std::make_pair(_binary->get_src2(), _flag));
                _this->SCEVMAP->insert({_phi, _scev});
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[0].emplace_back(std::make_pair(_binary->get_src2(), _flag));
                (*dims)[1].emplace_back(std::make_pair(_binary->get_src2(), _flag));
                _this->SCEVMAP->insert({_val, _scev});
                re = true;
            }
            else if (_isInVar(_binary->get_src1(), loop) && _binary->get_src2() == _phi)
            {
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[1].emplace_back(std::make_pair(_binary->get_src1(), _flag));
                _this->SCEVMAP->insert({_phi, _scev});
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[0].emplace_back(std::make_pair(_binary->get_src1(), _flag));
                (*dims)[1].emplace_back(std::make_pair(_binary->get_src1(), _flag));
                _this->SCEVMAP->insert({_val, _scev});
                re = true;
            }
            else if (_binary->get_src1() == _phi && (p_exp = _this->find_exp(_binary->get_src2())) != nullptr)
            {
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[1] = (*p_exp->get_dims())[0];
                (*dims)[2] = (*p_exp->get_dims())[1];
                _this->SCEVMAP->insert({_phi, _scev});
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[0].insert((*dims)[0].end(), (*dims)[1].begin(), (*dims)[1].end());
                (*dims)[0].insert((*dims)[0].end(), (*dims)[2].begin(), (*dims)[2].end());
                (*dims)[1] = (*p_exp->get_dims())[0];
                (*dims)[2] = (*p_exp->get_dims())[1];
                _this->SCEVMAP->insert({_val, _scev});
                re = true;
            }
            else if ((p_exp = _this->find_exp(_binary->get_src1())) != nullptr && _binary->get_src2() == _phi)
            {
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[1] = (*p_exp->get_dims())[0];
                (*dims)[2] = (*p_exp->get_dims())[1];
                _this->SCEVMAP->insert({_phi, _scev});
                _scev = new SCEVEXP();
                dims = _scev->get_dims();
                (*dims)[0].emplace_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
                (*dims)[0].insert((*dims)[0].end(), (*dims)[1].begin(), (*dims)[1].end());
                (*dims)[0].insert((*dims)[0].end(), (*dims)[2].begin(), (*dims)[2].end());
                (*dims)[1] = (*p_exp->get_dims())[0];
                (*dims)[2] = (*p_exp->get_dims())[1];
                _this->SCEVMAP->insert({_val, _scev});
                re = true;
            }
            break;
        default:
            break;
        }
    }
    return re;
}*/

void SCEV::LoopSCEVGen(Loop *lproot)
{
    for (Loop *_loop : *lproot->get_lpsons())
        LoopSCEVGen(_loop);
    if (lproot->get_header() == nullptr || !lproot->is_simple())
        return;

    bool target = true;
    while (target)
    {
        target = false;
        // target = _PHIAnalysis(lproot, this);
        target = target | PHIAnalysis(lproot, this);
        std::set<BasicBlock *> *_BBs = lproot->get_nwBBs();
        BasicBlock *_fa = nullptr;
        for (BasicBlock *_BB : *_BBs)
        {
            std::vector<Instrution *> *instrs = _BB->get_instrutions();
            for (Instrution *_instr : *instrs)
            {
                if (SCEVMAP->find(_instr) != SCEVMAP->end())
                    continue;
                Binary *p_instr;
                SCEVEXP *_scev = nullptr, *p_exp = nullptr, *p_exp2 = nullptr;
                bool flag = false;
                std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = nullptr;
                switch (_instr->get_Instrtype())
                {
                case InstrutionEnum::IMOD:
                    p_instr = (Binary *)_instr;
                    if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && _isInVar(p_instr->get_src2(), lproot))
                    {
                        _fa = ((Instrution *)p_instr->get_src1())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                            (*dims)[i] = (*p_exp->get_dims())[i];
                        _scev->set_mod(p_instr->get_src2());
                        SCEVMAP->insert({_instr, _scev});
                        _scev->set_mul(p_exp->is_mul());
                    }
                    break;
                case InstrutionEnum::IADD:
                case InstrutionEnum::ISUB:
                case InstrutionEnum::FADD:
                case InstrutionEnum::FSUB:
                    p_instr = (Binary *)_instr;
                    if (_instr->get_Instrtype() == InstrutionEnum::IADD || _instr->get_Instrtype() == InstrutionEnum::FADD)
                        flag = true;
                    else
                        flag = false;
                    if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && _isInVar(p_instr->get_src2(), lproot))
                    {
                        if (p_exp->is_mod())
                            break;
                        _fa = ((Instrution *)p_instr->get_src1())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                            (*dims)[i] = (*p_exp->get_dims())[i];
                        if (flag)
                            (*dims)[0].emplace_back(std::make_pair(p_instr->get_src2(), SCEVType::ADD));
                        else
                            (*dims)[0].emplace_back(std::make_pair(p_instr->get_src2(), SCEVType::SUB));
                        SCEVMAP->insert({_instr, _scev});
                    }
                    else if (_isInVar(p_instr->get_src1(), lproot) && (p_exp = find_exp(p_instr->get_src2())) != nullptr)
                    {
                        if (p_exp->is_mod())
                            break;
                        _fa = ((Instrution *)p_instr->get_src2())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                        {
                            if (flag)
                            {
                                (*dims)[i] = (*p_exp->get_dims())[i];
                                continue;
                            }

                            for (auto it : (*p_exp->get_dims())[i])
                            {
                                if (it.second == SCEVType::ADD)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::SUB));
                                else if (it.second == SCEVType::SUB)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::ADD));
                                else if (it.second == SCEVType::EXPADD0)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB0));
                                else if (it.second == SCEVType::EXPSUB0)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD0));
                                else if (it.second == SCEVType::EXPADD1)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB1));
                                else if (it.second == SCEVType::EXPSUB1)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD1));
                                else if (it.second == SCEVType::EXPADD2)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB2));
                                else if (it.second == SCEVType::EXPSUB2)
                                    (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD2));
                            }
                        }

                        (*dims)[0].emplace_back(std::make_pair(p_instr->get_src1(), SCEVType::ADD));
                        SCEVMAP->insert({_instr, _scev});
                    }
                    else if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && (p_exp2 = find_exp(p_instr->get_src2())) != nullptr)
                    {
                        if (p_exp->is_mod() || p_exp2->is_mod())
                            break;
                        _fa = ((Instrution *)p_instr->get_src1())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        _fa = ((Instrution *)p_instr->get_src2())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                        {
                            (*dims)[i] = (*p_exp->get_dims())[i];
                            (*dims)[i].insert((*dims)[i].end(), (*p_exp2->get_dims())[i].begin(), (*p_exp2->get_dims())[i].end());
                        }
                        SCEVMAP->insert({_instr, _scev});
                    }
                    break;
                case InstrutionEnum::IMUL:
                case InstrutionEnum::FMUL:
                    p_instr = (Binary *)_instr;
                    if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && _isInVar(p_instr->get_src2(), lproot))
                    {
                        if (p_exp->is_mod())
                            break;
                        _fa = ((Instrution *)p_instr->get_src1())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                        {
                            if ((*p_exp->get_dims())[i].empty())
                                break;
                            (*dims)[i] = (*p_exp->get_dims())[i];
                            (*dims)[i].emplace_back(std::make_pair(p_instr->get_src2(), SCEVType::MUL));
                        }
                        SCEVMAP->insert({_instr, _scev});
                        _scev->set_mul(true);
                    }
                    else if (_isInVar(p_instr->get_src1(), lproot) && (p_exp = find_exp(p_instr->get_src2())) != nullptr)
                    {
                        if (p_exp->is_mod())
                            break;
                        _fa = ((Instrution *)p_instr->get_src2())->get_parent();
                        if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                            break;
                        target = true;
                        _scev = new SCEVEXP();
                        dims = _scev->get_dims();
                        for (int i = 0; i < 5; ++i)
                        {
                            if ((*p_exp->get_dims())[i].empty())
                                break;
                            (*dims)[i] = (*p_exp->get_dims())[i];
                            (*dims)[i].emplace_back(std::make_pair(p_instr->get_src1(), SCEVType::MUL));
                        }
                        SCEVMAP->insert({_instr, _scev});
                        _scev->set_mul(true);
                    }
                    break;
                case InstrutionEnum::MINUS:
                    if ((p_exp = find_exp(((Unary *)_instr)->get_src())) == nullptr)
                        break;
                    if (p_exp->is_mod())
                        break;
                    _fa = ((Instrution *)((Unary *)_instr)->get_src())->get_parent();
                    if (lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
                        break;
                    _scev = new SCEVEXP();
                    dims = _scev->get_dims();
                    for (int i = 0; i < 5; ++i)
                    {
                        if ((*p_exp->get_dims())[i].empty())
                            break;
                        for (auto it : (*p_exp->get_dims())[i])
                        {
                            if (it.second == SCEVType::ADD)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::SUB));
                            else if (it.second == SCEVType::SUB)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::ADD));
                            else if (it.second == SCEVType::EXPADD0)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB0));
                            else if (it.second == SCEVType::EXPSUB0)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD0));
                            else if (it.second == SCEVType::EXPADD1)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB1));
                            else if (it.second == SCEVType::EXPSUB1)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD1));
                            else if (it.second == SCEVType::EXPADD2)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPSUB2));
                            else if (it.second == SCEVType::EXPSUB2)
                                (*dims)[i].emplace_back(std::make_pair(it.first, SCEVType::EXPADD2));
                        }
                    }
                    SCEVMAP->insert({_instr, _scev});
                    _scev->set_mul(p_exp->is_mul());
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void SCEV::PassRun(Module *_module)
{
    _Loop = new Loop_Analysis();
    _Loop->PassRun(_module);
    SCEVEXP::SCEVMAP = SCEVMAP;
    _module->print();
    for (Function *p_func : *_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        LoopSCEVGen(_Loop->get_LoopInfo()->find(p_func)->second);
    }
    SetStep();
    _module->print();
    print();
}

SCEVEXP::SCEVEXP()
    : dims(new std::vector<std::vector<std::pair<Value *, SCEVType>>>(5, std::vector<std::pair<Value *, SCEVType>>()))
{
}

SCEVEXP::~SCEVEXP()
{
    delete dims;
}

SCEV::SCEV()
    : SCEVMAP(new std::unordered_map<Value *, SCEVEXP *>)
{
}

SCEV::~SCEV()
{
    for (auto it : *SCEVMAP)
        delete it.second;
    delete _Loop;
    delete SCEVMAP;
}