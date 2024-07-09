#include <ir_opt/loopVarMove.hpp>

static bool inline _check(Value *p_val, Loop *loop)
{
    std::vector<Edge *> *edges = p_val->get_user_list();
    Instrution *p_instr = nullptr;
    int flag = 0;
    for (Edge *edge : *edges)
    {
        assert(is_a<Instrution>(edge->get_user()));
        p_instr = (Instrution *)edge->get_user();
        if (!loop->is_BBinLoop(p_instr->get_parent()))
            continue;
        if (p_instr->isCmp() && loop->get_lpCmp() == p_instr)
            continue;
        if (p_instr->get_Instrtype() != InstrutionEnum::PHINode)
            return true;
        else if (p_instr->get_user_list()->size() != 1)
            return true;
        if (p_instr->get_parent() != loop->get_header())
            return true;
        ++flag;
    }
    return flag <= 1 ? false : true;
}

static inline void _initStep(bool &flag, Loop *loop, SCEV *scev)
{
    if (flag)
        return;
    flag = true;
    BasicBlock *prev = loop->get_prev();
    SCEVEXP *p_exp = scev->find_exp(loop->get_lpStep());
    assert(p_exp != nullptr && !(*p_exp->get_dims())[0].empty() && !(*p_exp->get_dims())[1].empty());
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
    int _type = loop->get_cmpType();
    Value *p_offset = nullptr;
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
    const1 = new ConstantI32(1);
    prev->get_func()->value_pushBack(const1);
    Instrution *p_sub1 = new Binary(InstrutionEnum::ISUB, p_instr, const1, prev);
    loop->set_calSetp(p_instr, p_sub1);
}

static inline void _replace(Value *_instr, Value *newInstr, Loop *loop)
{
    std::vector<Edge *> *edges = _instr->get_user_list();
    std::vector<Edge *> nw;
    for (Edge *edge : *edges)
    {
        if (loop->is_BBinLoop(((Instrution *)(edge->get_user()))->get_parent()))
        {
            nw.push_back(edge);
            continue;
        }
        edge->set_val(newInstr);
    }
    _instr->get_user_list()->clear();
    for (Edge *p_edge : nw)
        _instr->get_user_list()->emplace_back(p_edge);
}

void loopVarMove::VarMove(Loop *loop, DomTree &_domtree)
{
    for (Loop *it : *loop->get_lpsons())
        VarMove(it, _domtree);
    if (loop->get_header() == nullptr || !loop->is_simple() || !loop->is_stepSet())
        return;
    // BasicBlock *prevBB = loop->get_prev();
    BasicBlock *prev = loop->get_prev();
    BasicBlock *_latch = (*loop->get_latchs()->begin());
    BasicBlock *_exiting = (*loop->get_exitings()->begin());
    bool is_under = false, init = false;
    Instrution *p_branch = prev->get_last_instrution();
    prev->Ins_popBack();
    for (BasicBlock *_BB : *loop->get_nwBBs())
    {
        std::vector<Instrution *> *instrs = _BB->get_instrs();
        if (instrs->empty())
            continue;
        if (!_domtree.is_dom(_BB, _latch))
            continue;
        if (_domtree.is_dom(_exiting, _BB) && _exiting != _BB)
            is_under = true;

        SCEVEXP *p_exp = nullptr;
        for (Instrution *_instr : (*instrs))
        {
            if ((p_exp = _scev->find_exp(_instr)) == nullptr || _check(_instr, loop))
                continue;
            if (p_exp->get_ToPhi() != nullptr)
                p_exp = _scev->find_exp(p_exp->get_ToPhi());
            else
                continue;
            _initStep(init, loop, _scev);
            Value *p_step = loop->get_calStep(is_under);
            std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = p_exp->get_dims();

            if ((*dims)[1].empty())
                assert(0);
            else if (((*dims)[2].empty()) || ((*dims)[2].begin()->second > SCEVType::MUL))
            {
                Value *src0 = p_exp->get_scr(0, prev);
                Value *src1 = p_exp->get_scr(1, prev);
                assert(src0->get_type()->get_type() == src1->get_type()->get_type());
                int flag = 0;
                if (src0->get_type()->get_type() == TypeEnum::F32)
                    flag = 5;
                Instrution *p_instr1 = new Binary((InstrutionEnum)(26 + flag), p_step, src1, prev);
                if (p_exp->is_mod())
                    p_instr1 = new Binary(InstrutionEnum::IMOD, p_instr1, p_exp->get_mod(), prev);
                Instrution *p_instr2 = new Binary((InstrutionEnum)(24 + flag), p_instr1, src0, prev);
                if (p_exp->is_mod())
                    p_instr2 = new Binary(InstrutionEnum::IMOD, p_instr2, p_exp->get_mod(), prev);
                _replace(_instr, p_instr2, loop);
            }
            else if (((*dims)[3].empty()) || ((*dims)[3].begin()->second > SCEVType::MUL))
            {
                Value *src0 = p_exp->get_scr(0, prev);
                Value *src1 = p_exp->get_scr(1, prev);
                Value *src2 = p_exp->get_scr(2, prev);
                assert(src0->get_type()->get_type() == src1->get_type()->get_type());
                int flag = 0;
                if (src0->get_type()->get_type() == TypeEnum::F32)
                    flag = 5;
                Instrution *p_instr1 = new Binary((InstrutionEnum)(26 + flag), p_step, src1, prev);
                if (p_exp->is_mod())
                    p_instr1 = new Binary(InstrutionEnum::IMOD, p_instr1, p_exp->get_mod(), prev);
                Instrution *p_instr2 = new Binary((InstrutionEnum)(24 + flag), p_instr1, src0, prev);
                if (p_exp->is_mod())
                    p_instr2 = new Binary(InstrutionEnum::IMOD, p_instr2, p_exp->get_mod(), prev);
                Value *const1 = new ConstantI32(1);
                prev->get_func()->value_pushBack(const1);
                Instrution *p_instr3 = new Binary((InstrutionEnum)(25 + flag), p_step, const1, prev);
                p_instr3 = new Binary((InstrutionEnum)(26 + flag), p_step, p_instr3, prev);
                Value *const2 = new ConstantI32(2);
                prev->get_func()->value_pushBack(const2);
                p_instr3 = new Binary((InstrutionEnum)(27 + flag), p_instr3, const2, prev);
                if (p_exp->is_mod())
                    p_instr3 = new Binary(InstrutionEnum::IMOD, p_instr3, p_exp->get_mod(), prev);
                if (flag)
                    p_instr3 = new Unary(InstrutionEnum::I2F, p_instr3, prev);
                p_instr3 = new Binary((InstrutionEnum)(26 + flag), src2, p_instr3, prev);
                p_instr3 = new Binary((InstrutionEnum)(24 + flag), p_instr2, p_instr3, prev);
                if (p_exp->is_mod())
                    p_instr3 = new Binary(InstrutionEnum::IMOD, p_instr3, p_exp->get_mod(), prev);
                _replace(_instr, p_instr3, loop);
            }
        }
    }
    prev->Ins_pushBack(p_branch);
}

void loopVarMove::PassRun(Module *p_module)
{
    _scev->PassRun(p_module);
    _Loop = _scev->_Loop;
    for (Function *p_func : *p_module->get_funcs())
    {
        if (p_func->get_blocks()->empty())
            continue;
        DomTree _tree(p_func);
        _tree.Run();
        VarMove(_Loop->get_LoopInfo()->find(p_func)->second, _tree);
    }
}

loopVarMove::loopVarMove()
{
    _scev = new SCEV();
}

loopVarMove::~loopVarMove()
{
    delete _scev;
}