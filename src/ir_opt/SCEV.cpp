#include <ir_opt/SCEV.hpp>

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

static bool inline _isInVar(Value *_val, Loop *loop)
{
    if (is_a<Constant>(_val) || loop->get_BBs()->find(((Instrution *)_val)->get_parent()) == loop->get_BBs()->end())
        return true;
    return false;
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
                        assert(0);
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
        std::vector<std::vector<std::pair<Value *, SCEVType>>> dims(5, std::vector<std::pair<Value *, SCEVType>>());
        for (int i = 0; i < 5; ++i)
            dims[i].clear();
        dims[0].push_back(std::make_pair(_phi->get_valueMap()->find(prev)->second->get_val(), SCEVType::ADD));
        switch (p_instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
        case InstrutionEnum::ISUB:
        case InstrutionEnum::FADD:
        case InstrutionEnum::FSUB:
            _dfs(_binary, loop, _this, dims, bits);
            if (!bits || bits >= 2000)
                break;
            _scev = new SCEVEXP();
            for (int i = 0; i < 4; ++i)
                (*_scev->get_dims())[i] = dims[i];
            re = true;
            _this->get_map()->insert({_phi, _scev});
            _scev = new SCEVEXP();
            for (int i = 0; i < 4; ++i)
                (*_scev->get_dims())[i] = dims[i];
            for (auto it : dims[1])
                (*_scev->get_dims())[0].emplace_back(it);
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
                    else if ((p_exp = find_exp(p_instr->get_src1())) != nullptr && (p_exp2 = find_exp(p_instr->get_src1())) != nullptr)
                    {
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