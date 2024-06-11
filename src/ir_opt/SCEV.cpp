#include <ir_opt/SCEV.hpp>

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

static inline Value *_getPhi(Value *_val)
{
    for (auto it : (*_val->get_user_list()))
    {
        if (((Instrution *)it->get_user())->get_Instrtype() == InstrutionEnum::PHINode)
            return it->get_user();
    }
    assert(0);
    return nullptr;
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
        loop->set_lpStep(_getPhi(p_cmp->get_src2()));
        loop->set_lpEnd(p_cmp->get_src1());
    }
    else if ((p_exp = find_exp(p_cmp->get_src1())) != nullptr && _isInVar(p_cmp->get_src2(), loop))
    {
        loop->set_lpStep(_getPhi(p_cmp->get_src1()));
        loop->set_lpEnd(p_cmp->get_src2());
    }
    else
        return;

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
        if (cnt)
            printf(", ");
        int cnt1 = 0;
        for (auto j : i)
        {
            if (cnt1)
                printf(" + ");
            switch (j.second)
            {
            case SCEVType::MUL:
                putchar('*');
                break;
            case SCEVType::SUB:
                putchar('-');
                break;
            default:
                break;
            }
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
    assert(is_a<Instrution>(_val));
    Instrution *p_instr = (Instrution *)_val;
    Binary *_binary = nullptr;
    bool flag;
    SCEVEXP *p_exp = nullptr;
    switch (p_instr->get_Instrtype())
    {
    case InstrutionEnum::PHINode:
        flags += 1000;
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
                flag += 3000;
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
                    else
                        flags += 3000;
                }
            }
            if (!(*p_exp->get_dims())[4].empty())
                flag += 3000;
        }
        else
        {
            if (flag)
                flags += 3000;
            _dfs(_binary->get_src2(), loop, _this, dims, flags);
        }
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
        assert(is_a<Instrution>(_val));
        Instrution *p_instr = (Instrution *)_val;
        if (loop->get_nwBBs()->find(p_instr->get_parent()) == loop->get_nwBBs()->end())
            continue;
        if (!is_a<Binary>(_val))
            continue;
        Binary *_binary = (Binary *)p_instr;
        SCEVEXP *_scev;
        int bits = 0;
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
            _dfs(_binary, loop, _this, dims, bits);
            if (!bits || bits >= 2000)
                break;
            _scev = new SCEVEXP();
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
                        if (lproot->is_BBinLoop(_fa) && lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
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
                        if (lproot->is_BBinLoop(_fa) && lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
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
                        if (lproot->is_BBinLoop(_fa) && lproot->get_nwBBs()->find(_fa) == lproot->get_nwBBs()->end())
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
                            }
                        }

                        (*dims)[0].emplace_back(std::make_pair(p_instr->get_src1(), SCEVType::ADD));
                        SCEVMAP->insert({_instr, _scev});
                    }
                    else if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && (p_exp2 = find_exp(p_instr->get_src2())) != nullptr)
                    {
                        if (p_exp->is_mod() || p_exp2->is_mod())
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
                    if ((p_exp = find_exp(_instr)) == nullptr)
                        break;
                    if (p_exp->is_mod())
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