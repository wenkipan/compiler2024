#include "../../include/ir_opt/loop.hpp"

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
    prevphi->print();
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
    puts("YYYYYYYYYYYYYYY");
    func->print();
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
    p_module->print();
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