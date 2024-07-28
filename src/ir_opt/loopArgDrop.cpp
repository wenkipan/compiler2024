#include "../../include/ir_opt/loopArgDrop.hpp"
#include "../../include/ir_opt/SCCP.hpp"
#include "../../include/ir_opt/DCE.hpp"
#include "../../include/ir_opt/SimplifyCFG.hpp"

static bool inline _check(Value *p_val, BasicBlock *BB)
{
    std::vector<Edge *> *edges = p_val->get_user_list();
    for (Edge *edge : *edges)
    {
        Instrution *p_user = (Instrution *)(edge->get_user());
        if (p_user->get_parent() != BB)
            return true;
    }
    return false;
}

static inline void _initStep(Loop *loop, SCEV *scev)
{
    BasicBlock *prev = loop->get_prev();
    SCEVEXP *p_exp = scev->find_exp(loop->get_lpStep());
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

void LoopArgDrop::DealLoop(Loop *loop)
{
    for (auto son : *loop->get_lpsons())
        DealLoop(son);
    if (!loop->get_lpsons()->empty() || loop->get_BBs()->size() != 1 || !loop->is_stepSet())
        return;
    BasicBlock *_BB = loop->get_header();
    if (_BB->get_phinodes()->size() != 1)
        return;
    PHINode *phi = *_BB->get_phinodes()->begin();
    Value *val = phi->get_valueMap()->find(_BB)->second->get_val();
    SCEVEXP *exp = scev->find_exp(val);
    if (exp == nullptr)
        return;
    std::vector<std::vector<std::pair<Value *, SCEVType>>> *dims = exp->get_dims();
    if (!(*dims)[2].empty() || !(*dims)[3].empty())
        return;
    auto usrs = val->get_user_list();
    int usrcnt = 0;
    for (auto edge : *usrs)
    {
        Instrution *user = (Instrution *)edge->get_user();
        if (loop->is_BBinLoop(user->get_parent()))
        {
            if (!(user->isCmp() || user->isPHINode()))
                return;
            ++usrcnt;
        }
    }
    if (usrcnt != 2)
        return;
    for (Instrution *p_instr : *_BB->get_instrs())
    {
        if (p_instr->get_Instrtype() == InstrutionEnum::Call || p_instr->get_Instrtype() == InstrutionEnum::Store || p_instr->get_Instrtype() == InstrutionEnum::Load)
            return;
        if (val != p_instr && _check(p_instr, _BB))
            return;
    }
    if (_check((*_BB->get_phinodes()->begin()), _BB))
        return;
    assert(_BB->get_last_instrution()->isBranch());
    Instrution *Pbranch = loop->get_prev()->get_last_instrution();
    loop->get_prev()->Ins_popBack();
    _initStep(loop, scev);
    Value *src0 = exp->get_scr(0, loop->get_prev());
    Value *src1 = exp->get_scr(1, loop->get_prev());
    assert(src0->get_type()->get_type() == src1->get_type()->get_type());
    int flag = 0;
    if (src0->get_type()->get_type() == TypeEnum::F32)
        flag = 5;
    Value *p_step = loop->get_calStep(true);
    Instrution *p_instr1 = new Binary((InstrutionEnum)(26 + flag), p_step, src1, loop->get_prev());
    Instrution *p_instr2 = new Binary((InstrutionEnum)(24 + flag), p_instr1, src0, loop->get_prev());
    _replace(val, p_instr2, loop);
    Branch *p_branch = (Branch *)_BB->get_last_instrution();
    _BB->Ins_popBack();
    bool Texit = true;
    if (p_branch->get_trueBB() == _BB)
        Texit = false;
    Constant *num0 = new ConstantI32(0);
    _BB->get_func()->value_pushBack(num0);
    Instrution *p_cmp = _BB->get_last_instrution();
    assert(p_branch->get_cond() == p_cmp);
    Cmp *n_cmp = nullptr;
    if (Texit)
        n_cmp = new Cmp(InstrutionEnum::IEQ, num0, num0, _BB);
    else
        n_cmp = new Cmp(InstrutionEnum::INEQ, num0, num0, _BB);
    (*p_branch->get_value_list()->begin())->set_val(n_cmp);
    assert(p_cmp->get_user_list()->size() == 1);
    p_cmp->get_user_list()->clear();
    p_cmp->drop();
    _BB->Ins_pushBack(p_branch);
    loop->get_prev()->Ins_pushBack(Pbranch);
}

void LoopArgDrop::PassRun(Module *p_module)
{
    scev->PassRun(p_module);
    LA = scev->_Loop;
    for (Function *p_func : (*p_module->get_funcs()))
    {
        if (p_func->get_blocks()->empty())
            continue;
        Loop *root = LA->get_LoopInfo()->find(p_func)->second;
        for (Loop *it : (*root->get_lpsons()))
            DealLoop(it);
        SCCP sccp;
        sccp.run(p_func);
        DCE dce;
        dce.run(p_func);
        SimplifyCFG spycfg;
        spycfg.run(p_func);
    }
}

LoopArgDrop::LoopArgDrop()
{
    scev = new SCEV();
}

LoopArgDrop::~LoopArgDrop()
{
    delete scev;
}