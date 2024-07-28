#include "backend/rv/RV.hpp"
#include "backend/rv/RVstand.hpp"
#include "ir/Constant.hpp"
#include "ir/Type.hpp"
#include <backend/rv/RVGen.hpp>
#include <backend/rv/RVImmeTrans.hpp>

static inline RVInstr *gen_instr_op2_before(RVENUM e, RVOperand *o1, RVOperand *o2, RVBlock *b, RVInstr *pos = nullptr)
{
    auto newi = new RVInstr(e);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    b->instr_insert_before(pos, newi);
    return newi;
}
static inline RVInstr *gen_instr_op3_before(RVENUM e, RVOperand *o1, RVOperand *o2, RVOperand *o3, RVBlock *b, RVInstr *pos = nullptr)
{
    auto newi = new RVInstr(e);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    newi->ops_push_back(o3);
    b->instr_insert_before(pos, newi);
    return newi;
}

void RVGen::init(Module *m)
{
    rvm = new RVModule(m->get_infile(), m->get_outfile());
    for (auto f : *m->get_funcs())
    {
        auto newf = init_f(f);
        rvm->funcs_push_back(newf);
    }
    for (auto GV : *m->get_globals())
    {
        assert(GV->get_type()->get_type() == TypeEnum::Ptr);
        Type *pointeetype = ((Ptr *)GV->get_type())->get_btype();
        RVGlobalVariable *newgv = new RVGlobalVariable(GV->get_name());
        if (pointeetype->get_type() == TypeEnum::I32 || pointeetype->get_type() == TypeEnum::F32)
        {
            if (GV->get_init())
            {
                if (is_a<ConstantI32>(GV->get_init()))
                {
                    newgv->words_push_back(((ConstantI32 *)GV->get_init())->get_32_at(0));
                }
                else if (is_a<ConstantF32>(GV->get_init()))
                {
                    float f = ((ConstantF32 *)GV->get_init())->get_32_at(0);
                    newgv->words_push_back(*(uint32_t *)(&f));
                }
            }
            newgv->set_space(4);
        }
        else if (pointeetype->get_type() == TypeEnum::Array)
        {
            newgv->set_space(4 * ((ArrayType *)pointeetype)->get_size());
            if (GV->get_init())
            {
                if (is_a<ConstantI32>(GV->get_init()))
                    for (auto i : ((ConstantI32 *)GV->get_init())->get_i32())
                        newgv->words_push_back(i);
                else if (is_a<ConstantF32>(GV->get_init()))
                    for (auto f : ((ConstantF32 *)GV->get_init())->get_f32())
                        newgv->words_push_back(*(uint32_t *)(&f));
            }
        }
        else
            assert(0);
        rvm->globals_push_back(newgv);
        val2valmap.emplace(GV, newgv);
    }
}
RVFunc *RVGen::init_f(Function *f)
{
    auto newf = new RVFunc(f->get_name());
    auto newinfo = new funcInfo();
    infomap.emplace(f, newinfo);
    val2valmap.emplace(f, newf);
    newinfo->rv_f = newf;
    newinfo->link_f = f;

    // params to stack
    std::vector<Param *> patostack;
    int icnt = 8, fcnt = 8;
    for (auto pa : *f->get_params())
    {
        if (pa->get_type()->get_type() == TypeEnum::I32 || pa->get_type()->get_type() == TypeEnum::Ptr)
        {
            if (icnt)
                icnt--;
            else
                patostack.push_back(pa);
        }
        else if (pa->get_type()->get_type() == TypeEnum::F32)
        {
            if (fcnt)
                fcnt--;
            else
                patostack.push_back(pa);
        }
        else
            assert(0);
    }
    newinfo->paramstostack = patostack;
    if (f->get_isExternal())
    {
        newf->set_external();
        return newf;
    }

    // ra
    SSARegisterAlloc *s = new SSARegisterAlloc;
    s->run(f);
    newinfo->ssara = s;

    for (auto BB : *f->get_blocks())
    {
        std::string na;
        na = "." + BB->get_func()->get_name() + "_b" + std::to_string(BB->get_ID());
        RVBlock *newb = new RVBlock(na);
        val2valmap.emplace(BB, newb);
        newf->blocks_push_back(newb);
    }
    // build edge
    for (auto BB : *f->get_blocks())
    {
        for (auto succ : successors(BB))
        {
            new RVEdge(getrvval(BB), getrvval(succ));
        }
    }
    return newf;
}
RVValue *RVGen::getrvval(Value *a)
{
    assert(val2valmap.find(a) != val2valmap.end());
    return val2valmap.find(a)->second;
}
void RVGen::set_offset(Value *i, int off)
{
    if (alloc_and_param_offset_map.find(i) == alloc_and_param_offset_map.end())
        alloc_and_param_offset_map.emplace(i, off);
    else
        alloc_and_param_offset_map.find(i)->second = off;
}
int RVGen::get_offset(Value *addr)
{
    if (alloc_and_param_offset_map.find(addr) == alloc_and_param_offset_map.end())
    {
        assert(is_a<Alloca>(addr));
        assert(ssara->whichPara((Alloca *)addr));
        assert(alloc_and_param_offset_map.find(ssara->whichPara((Alloca *)addr)) != alloc_and_param_offset_map.end());
        return alloc_and_param_offset_map.find(ssara->whichPara((Alloca *)addr))->second;
    }
    return alloc_and_param_offset_map.find(addr)->second;
}
void RVGen::run(Module *m)
{
    init(m);
    for (auto f : *m->get_funcs())
    {
        if (f->get_isExternal())
            continue;
        gen_fnc(f);
    }

    RVImmeTrans after;
    after.run(rvm);
    rvm->print(1);
}
void RVGen::gen_fnc(Function *f)
{
    printf("---------after ssa\n");
    f->print();

    ssara = infomap.find(f)->second->ssara;
    cur_b = (RVBlock *)getrvval(f->get_entryBB());
    gen_sp_sub_and_offset_for_alloc_and_param(f);

    for (auto BB : *f->get_blocks())
    {
        cur_b = (RVBlock *)getrvval(BB);
        for (auto instr : *BB->get_instrs())
            gen_instr(instr);
    }

    gen_sp_add_and_pop(f);
}
static inline bool find_if_call(Function *f)
{
    for (auto B : *f->get_blocks())
        for (auto i : *B->get_instrs())
            if (i->isCall())
                return 1;
    return 0;
}
void RVGen::gen_sp_sub_and_offset_for_alloc_and_param(Function *f)
{
    std::set<int> used_reg;
    for (auto BB : *f->get_blocks())
    {
        for (auto instr : *BB->get_instrs())
        {
            if (is_a<Move>(instr))
                used_reg.emplace(ssara->getReg(instr->get_operand_at(0)));
            else if (ssara->getReg(instr) != -1)
                used_reg.emplace(ssara->getReg(instr));
        }
    }
    std::vector<int> topushstack;
    for (auto k : used_reg)
    {
        if (k >= zero && k <= tp && k != ra)
            assert(0);
        else if (s0 == k || s1 == k || (s2 <= k && k <= s11))
            topushstack.push_back(k);
        else if (fs0 == k || fs1 == k || (fs2 <= k && k <= fs11))
            topushstack.push_back(k);
        else if (k > 63 || k < 0)
            assert(0);
    }
    // int if_ra = 0;
    if (find_if_call(f))
    {
        // if_ra = 1;
        topushstack.push_back(ra);
    }
    infomap.find(f)->second->regs_need_resume = topushstack;

    int pushedsize = 8 * (topushstack.size());
    infomap.find(f)->second->pushedsize = pushedsize;

    int max_pushed_callee_param = find_max_pushed_callee_param(f);
    infomap.find(f)->second->max_pushed_callee_param_and_resume = max_pushed_callee_param;

    int allocsize = find_all_alloc_size(f);
    infomap.find(f)->second->allocsize = allocsize;

    int sp_sub_offset = allocsize + max_pushed_callee_param + pushedsize;
    if ((sp_sub_offset) % 16) // allgin
    {
        printf("-----MOD!!!%d\n", sp_sub_offset);
        sp_sub_offset += (16 - ((sp_sub_offset) % 16));
        printf("-----MOD!!!%d\n", sp_sub_offset);
    }
    infomap.find(f)->second->sp_sub_offset = sp_sub_offset;

    // no subi to use
    if (sp_sub_offset != 0)
        gen_instr_op3_before(RVENUM::rv_add, new RVReg(sp), new RVReg(sp), new RVImme(-sp_sub_offset), cur_b);

    int regpos = 0;
    for (auto regno : topushstack)
        gen_instr_op2_before(RVENUM::rv_s, new RVReg_d(regno),
                             new RVReg_off(sp, allocsize + max_pushed_callee_param + 8 * (regpos++)), cur_b);

    int papos = 0;
    for (auto pa : infomap.find(f)->second->paramstostack)
    {
        printf("setpaoff:%d  ", pushedsize + sp_sub_offset + 8 * papos);
        pa->print();
        printf("\n");
        set_offset(pa, sp_sub_offset + 8 * (papos++));
    }

    // set offset for allocs
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
            if (is_a<Alloca>(instr))
                set_offset(instr, max_pushed_callee_param + get_offset(instr));
}
int RVGen::find_max_pushed_callee_param(Function *f)
{
    int max_pushed_callee_param = 0;
    for (auto B : *f->get_blocks())
        for (auto i : *B->get_instrs())
            if (i->isCall())
            {
                assert(infomap.find(f) != infomap.end());
                assert(infomap.find((Function *)i->get_operand_at(0)) != infomap.end());
                int wqzhemechang = ssara->regsStillAliveAfterCall((Call *)i).size();
                int parmstostack = infomap.find((Function *)i->get_operand_at(0))->second->paramstostack.size();
                printf("wqzhemechang:%d\n", wqzhemechang);
                printf("wqzhemechang:%d\n", parmstostack);
                if (max_pushed_callee_param < wqzhemechang + parmstostack)
                    max_pushed_callee_param = wqzhemechang + parmstostack;
            }
    return max_pushed_callee_param * 8;
}
int RVGen::find_all_alloc_size(Function *f)
{
    int allocsize = 0;
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
            if (is_a<Alloca>(instr))
            {
                printf("--alllocsize");
                printf("%d", allocsize);
                printf("----alloc----");
                instr->print();
                set_offset(instr, allocsize);
                ((Alloca *)instr)->get_type()->print();
                assert(((Alloca *)instr)->get_type()->get_type() == TypeEnum::Ptr);
                Ptr *ptr = (Ptr *)((Alloca *)instr)->get_type();
                if (ptr->get_btype()->isArray())
                    allocsize += 4 * ((ArrayType *)ptr->get_btype())->get_size();
                else if (ptr->get_btype()->get_type() == TypeEnum::Ptr)
                {
                    allocsize += 8;
                }
                else
                    allocsize += 4;
            }
    return allocsize;
}

void RVGen::gen_sp_add_and_pop(Function *f)
{
    cur_b = (RVBlock *)getrvval(f->get_retBB());

    funcInfo *info = infomap.find(f)->second;
    int off = info->allocsize + info->max_pushed_callee_param_and_resume;
    int regpos = 0;
    for (auto regno : info->regs_need_resume)
        gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(regno),
                             new RVReg_off(sp, off + 8 * (regpos++)), cur_b, cur_b->get_last_instr());

    int sp_sub_offset = info->sp_sub_offset;
    if (sp_sub_offset != 0)
        gen_instr_op3_before(RVENUM::rv_add, new RVReg(sp), new RVReg(sp),
                             new RVImme(sp_sub_offset), cur_b, cur_b->get_last_instr());
}

void RVGen::gen_instr(Instrution *i)
{
    printf("GENINSTR::");
    i->print();
    if (i->isAlloca())
    {
        return;
    }
    else if (i->isBinary())
    {
        gen_binary(i);
    }
    else if (i->isUnary())
        gen_unary(i);
    else if (i->isCall())
    {
        gen_call(i);
    }
    else if (i->isReturn())
    {
        gen_ret(i);
    }
    else if (i->isBranch())
    {
        gen_branch(i);
    }
    else if (i->isJmp())
    {
        gen_jmp(i);
    }
    else if (i->isCmp())
    {
        if (((Cmp *)i)->isCond())
            return;
        gen_cmp(i);
    }
    else if (i->isLoad())
    {
        gen_load(i);
    }
    else if (i->isStore())
    {
        gen_store(i);
    }
    else if (i->isGEP())
    {
        assert(0);
    }
    else if (i->isPHINode())
    {
        assert(0);
    }
    else if (i->isMove())
    {
        auto mv = gen_mov(i->get_operand_at(0), i->get_operand_at(1));
        val2valmap.emplace(i, mv);
    }
    else
        assert(0);
}
RVOperand *RVGen::get_op(Value *i, RVInstr *pos = nullptr)
{
    if (is_a<ConstantI32>(i))
    {
        return new RVImme(((ConstantI32 *)i)->get_32_at(0));
    }
    else if (is_a<ConstantF32>(i))
    {
        return new RVImmef(((ConstantF32 *)i)->get_32_at(0));
    }
    else if (is_a<Alloca>(i)) // for geps
    {
        gen_instr_op3_before(RVENUM::rv_add, new RVReg(t0), new RVReg(sp), new RVImme(get_offset(i)), cur_b, pos);
        return new RVReg_d(t0);
    }
    else if (is_a<GlobalVariable>(i))
    {
        assert(0);
        // gen_load_GV_addr(ssara->getReg(i), i, cur_b);
        // return new RVReg(ssara->getReg(i));
    }
    else
    {
        if (i->get_type()->get_type() == TypeEnum::F32)
            assert(rv_is_f_reg(ssara->getReg(i)));
        if (i->get_type()->get_type() == TypeEnum::Ptr)
            return new RVReg_d(ssara->getReg(i));
        return new RVReg(ssara->getReg(i));
    }
}
RVOperand *RVGen::get_op(Value *i, bool call_use, RVInstr *pos = nullptr)
{
    assert(call_use);
    if (is_a<ConstantI32>(i))
    {
        gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), new RVImme(((ConstantI32 *)i)->get_32_at(0)), cur_b);
        return new RVReg_d(t0);
    }
    else if (is_a<ConstantF32>(i))
    {
        return new RVImmef(((ConstantF32 *)i)->get_32_at(0));
    }
    else if (is_a<Alloca>(i)) // for geps
    {
        gen_instr_op3_before(RVENUM::rv_add, new RVReg(t0), new RVReg(sp), new RVImme(get_offset(i)), cur_b, pos);
        return new RVReg_d(t0);
    }
    else if (is_a<GlobalVariable>(i))
    {
        assert(0);
        // gen_load_GV_addr(ssara->getReg(i), i, cur_b);
        // return new RVReg(ssara->getReg(i));
    }
    else
    {
        if (i->get_type()->get_type() == TypeEnum::F32)
            assert(rv_is_f_reg(ssara->getReg(i)));
        if (i->get_type()->get_type() == TypeEnum::Ptr)
            return new RVReg_d(ssara->getReg(i));
        return new RVReg_d(ssara->getReg(i));
    }
}
void RVGen::gen_binary(Instrution *i)
{
    assert(i->get_value_list()->size() == 2);
    RVOperand *dst = get_op(i);
    RVOperand *op1 = get_op(i->get_operand_at(0));
    RVOperand *op2 = get_op(i->get_operand_at(1));

    assert(!(is_a<RVImme>(op1) && is_a<RVImme>(op2)));
    assert(!(is_a<RVImmef>(op1) && is_a<RVImmef>(op2)));

    switch (i->get_Instrtype())
    {
    case InstrutionEnum::IADD:
    case InstrutionEnum::FADD:
        if (is_a<RVImme>(op1) || is_a<RVImmef>(op1))
            std::swap(op1, op2);
        gen_instr_op3_before(RVENUM::rv_add, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::ISUB:
    case InstrutionEnum::FSUB:
        if (is_a<RVImme>(op2))
        {
            delete op2;
            gen_instr_op3_before(RVENUM::rv_add, dst, op1,
                                 new RVImme(-((ConstantI32 *)(i->get_operand_at(1)))->get_32_at(0)), cur_b);
        }
        else
            gen_instr_op3_before(RVENUM::rv_sub, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::SHL:
        gen_instr_op3_before(RVENUM::rv_sll, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::LSHR:
        gen_instr_op3_before(RVENUM::rv_srl, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::IMUL:
    case InstrutionEnum::FMUL:
        gen_instr_op3_before(RVENUM::rv_mul, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::IDIV:
    case InstrutionEnum::FDIV:
        gen_instr_op3_before(RVENUM::rv_div, dst, op1, op2, cur_b);
        return;
    case InstrutionEnum::IMOD:
        gen_instr_op3_before(RVENUM::rv_rem, dst, op1, op2, cur_b);
        break;
    default:
        assert(0);
        break;
    }
}
void RVGen::gen_unary(Instrution *i)
{
    assert(i->get_value_list()->size() == 1);
    Value *src1 = i->get_operand_at(0);
    switch (i->get_Instrtype())
    {
    case InstrutionEnum::Assign:
        gen_mov(i, src1);
        break;
    case InstrutionEnum::MINUS:
        assert(!is_a<Constant>(src1));
        gen_instr_op2_before(RVENUM::rv_neg, new RVReg(ssara->getReg(i)), new RVReg(ssara->getReg(src1)), cur_b);
        break;
    case InstrutionEnum::F2I:
        assert(rv_is_r_reg(ssara->getReg(i)));
        assert(rv_is_f_reg(ssara->getReg(src1)));
        // gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ft0), new RVReg(ssara->getReg(src1)), cur_b);
        // gen_instr_op2_before(RVENUM::rv_fcvt_w_s, new RVReg(ft0), new RVReg(ft0), cur_b);
        // gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ssara->getReg(i)), new RVReg(ft0), cur_b);
        gen_instr_op2_before(RVENUM::rv_fcvt_w_s, new RVReg(ssara->getReg(i)), new RVReg(ssara->getReg(src1)), cur_b);
        break;
    case InstrutionEnum::I2F:
        assert(rv_is_f_reg(ssara->getReg(i)));
        assert(rv_is_r_reg(ssara->getReg(src1)));
        // gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ft0), new RVReg(ssara->getReg(src1)), cur_b);
        gen_instr_op2_before(RVENUM::rv_fcvt_s_w, new RVReg(ssara->getReg(i)), new RVReg(ssara->getReg(src1)), cur_b);
        break;
    default:
        assert(0);
        break;
    }
}
void RVGen::gen_ret(Instrution *i)
{
    // mov to s0/r0
    if (i->get_type()->get_type() == TypeEnum::Void)
    {
    }
    else
    {
        Value *src = i->get_operand_at(0);
        if (is_a<ConstantI32>(src))
        {
            gen_instr_op2_before(RVENUM::rv_l, new RVReg(a0), new RVImme(((ConstantI32 *)src)->get_32_at(0)), cur_b);
        }
        else if (is_a<ConstantF32>(src))
        {
            gen_instr_op2_before(RVENUM::rv_l, new RVReg(fa0), new RVImmef(((ConstantF32 *)src)->get_32_at(0)), cur_b);
        }
        else if (rv_is_r_reg(ssara->getReg(src)))
        {
            if (ssara->getReg(src) != a0)
                gen_instr_op2_before(RVENUM::rv_mv, new RVReg(a0), new RVReg(ssara->getReg(src)), cur_b);
        }
        else if (rv_is_f_reg(ssara->getReg(src)))
        {
            if (ssara->getReg(src) != fa0)
                gen_instr_op2_before(RVENUM::rv_mv, new RVReg(fa0), new RVReg(ssara->getReg(src)), cur_b);
        }
        else
            assert(0);
    }

    auto newi = new RVInstr(RVENUM::rv_ret);
    cur_b->instr_insert_before(nullptr, newi);
}

void RVGen::gen_branch(Instrution *i)
{
    BasicBlock *parent = i->get_parent();
    Cmp *cmp = (Cmp *)i->get_operand_at(0);
    // gen cmp
    assert(cmp->get_operand_at(0)->get_type()->get_type() == cmp->get_operand_at(1)->get_type()->get_type());

    // gen b1
    BasicBlock *truebb = (BasicBlock *)parent->get_user_list()->at(0)->get_user();
    BasicBlock *falsebb = (BasicBlock *)parent->get_user_list()->at(1)->get_user();
    RVInstr *newi;
    switch (cmp->get_Instrtype())
    {
    case InstrutionEnum::IEQ:
        newi = new RVInstr(RVENUM::rv_b, RVcond::eq);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::INEQ:
        newi = new RVInstr(RVENUM::rv_b, RVcond::ne);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::IGT:
        newi = new RVInstr(RVENUM::rv_b, RVcond::gt);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::IGE:
        newi = new RVInstr(RVENUM::rv_b, RVcond::ge);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::ILT:
        newi = new RVInstr(RVENUM::rv_b, RVcond::lt);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::ILE:
        newi = new RVInstr(RVENUM::rv_b, RVcond::le);
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        break;
    case InstrutionEnum::FEQ:
        // careful
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::eq);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(new RVAddr((RVLable *)getrvval(falsebb)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_j);
        newi->ops_push_back(new RVAddr((RVLable *)getrvval(truebb)));
        cur_b->instr_insert_before(nullptr, newi);
        return;
    case InstrutionEnum::FNEQ:
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::eq);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        break;
    case InstrutionEnum::FGT:
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::le);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        break;
    case InstrutionEnum::FGE:
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::lt);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        break;
    case InstrutionEnum::FLT:
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::ge);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        break;
    case InstrutionEnum::FLE:
        newi = new RVInstr(RVENUM::rv_fcmp, RVcond::gt);
        newi->ops_push_back(new RVReg(t0));
        newi->ops_push_back(get_op(cmp->get_operand_at(0)));
        newi->ops_push_back(get_op(cmp->get_operand_at(1)));
        cur_b->instr_insert_before(nullptr, newi);

        newi = new RVInstr(RVENUM::rv_b, RVcond::eqz);
        newi->ops_push_back(new RVReg(t0));
        break;
    default:
        assert(0);
        break;
    }
    newi->ops_push_back(new RVAddr((RVLable *)getrvval(truebb)));
    cur_b->instr_insert_before(nullptr, newi);
    // gen b2
    newi = new RVInstr(RVENUM::rv_j);
    newi->ops_push_back(new RVAddr((RVLable *)getrvval(falsebb)));
    cur_b->instr_insert_before(nullptr, newi);
}

void RVGen::gen_jmp(Instrution *i)
{
    BasicBlock *parent = i->get_parent();
    BasicBlock *thenbb = (BasicBlock *)parent->get_user_list()->at(0)->get_user();
    auto newi = new RVInstr(RVENUM::rv_j);
    newi->ops_push_back(new RVAddr((RVLable *)getrvval(thenbb)));
    cur_b->instr_push_back(newi);
}

void RVGen::gen_cmp(Instrution *cmp)
{
    printf("-----cmptype\n");
    cmp->get_operand_at(1)->get_type()->print();
    fflush(stdout);
    TypeEnum ty1 = cmp->get_operand_at(0)->get_type()->get_type();
    TypeEnum ty2 = cmp->get_operand_at(1)->get_type()->get_type();
    if (ty1 == TypeEnum::I1)
        ty1 = TypeEnum::I32;
    if (ty2 == TypeEnum::I1)
        ty2 = TypeEnum::I32;
    assert(ty1 == ty2);

    if (ty1 == TypeEnum::F32)
    {
        auto newi = gen_instr_op3_before(RVENUM::rv_fcmp, get_op(cmp), get_op(cmp->get_operand_at(0)), get_op(cmp->get_operand_at(1)), cur_b);
        switch (cmp->get_Instrtype())
        {
        case InstrutionEnum::FEQ:
            newi->set_cond(RVcond::eq);
            break;
        case InstrutionEnum::FNEQ:
            newi->set_cond(RVcond::eq);
            gen_instr_op3_before(RVENUM::rv_xor, get_op(cmp), get_op(cmp), new RVImme(1), cur_b);
            break;
        case InstrutionEnum::FGT:
            newi->set_cond(RVcond::gt);
            break;
        case InstrutionEnum::FGE:
            newi->set_cond(RVcond::ge);
            break;
        case InstrutionEnum::FLT:
            newi->set_cond(RVcond::lt);
            break;
        case InstrutionEnum::FLE:
            newi->set_cond(RVcond::le);
            break;
        default:
            assert(0);
            break;
        }
    }
    else if (ty2 == TypeEnum::I32)
    {
        switch (cmp->get_Instrtype())
        {
        case InstrutionEnum::IEQ:
            gen_instr_op3_before(RVENUM::rv_xor, get_op(cmp), get_op(cmp->get_operand_at(0)), get_op(cmp->get_operand_at(1)), cur_b);
            gen_instr_op2_before(RVENUM::rv_seqz, get_op(cmp), get_op(cmp), cur_b);
            break;
        case InstrutionEnum::INEQ:
            gen_instr_op3_before(RVENUM::rv_xor, get_op(cmp), get_op(cmp->get_operand_at(0)), get_op(cmp->get_operand_at(1)), cur_b);
            gen_instr_op2_before(RVENUM::rv_snez, get_op(cmp), get_op(cmp), cur_b);
            break;
        case InstrutionEnum::IGT:
            gen_instr_op3_before(RVENUM::rv_slt, get_op(cmp), get_op(cmp->get_operand_at(1)), get_op(cmp->get_operand_at(0)), cur_b);
            break;
        case InstrutionEnum::IGE:
            gen_instr_op3_before(RVENUM::rv_slt, get_op(cmp), get_op(cmp->get_operand_at(0)), get_op(cmp->get_operand_at(1)), cur_b);
            gen_instr_op3_before(RVENUM::rv_xor, get_op(cmp), get_op(cmp), new RVImme(1), cur_b);
            break;
        case InstrutionEnum::ILT:
            gen_instr_op3_before(RVENUM::rv_slt, get_op(cmp), get_op(cmp->get_operand_at(0)), get_op(cmp->get_operand_at(1)), cur_b);
            break;
        case InstrutionEnum::ILE:
            gen_instr_op3_before(RVENUM::rv_slt, get_op(cmp), get_op(cmp->get_operand_at(1)), get_op(cmp->get_operand_at(0)), cur_b);
            gen_instr_op3_before(RVENUM::rv_xor, get_op(cmp), get_op(cmp), new RVImme(1), cur_b);
            break;
        default:
            assert(0);
            break;
        }
    }
    else
        assert(0);
}
void RVGen::gen_load_GV_addr(int rno, Value *addr)
{
    gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(rno), new RVAddr((RVLable *)getrvval(addr)), cur_b);
}
RVReg_off *RVGen::get_op_addr(Value *addr)
{
    if (is_a<GlobalVariable>(addr))
    {
        gen_load_GV_addr(ssara->getReg(addr), addr);
        return new RVReg_off(ssara->getReg(addr), 0);
    }
    else if (is_a<Alloca>(addr))
    {
        return new RVReg_off(sp, get_offset(addr));
    }
    else // gep results
        return new RVReg_off(ssara->getReg(addr), 0);
}
void RVGen::gen_load(Instrution *i)
{
    Value *addr = i->get_operand_at(0);
    if (ssara->whichGV(dynamic_cast<Alloca *>(addr)))
        gen_load_GV_addr(ssara->getReg(i), ssara->whichGV(dynamic_cast<Alloca *>(addr)));
    else
        gen_instr_op2_before(RVENUM::rv_l, get_op(i), get_op_addr(addr), cur_b);
}
void RVGen::gen_store(Instrution *i)
{
    Value *stored = i->get_operand_at(1);
    Value *addr = i->get_operand_at(0);
    gen_instr_op2_before(RVENUM::rv_s, get_op(stored), get_op_addr(addr), cur_b);
}
RVInstr *RVGen::gen_mov(Value *dst, Value *src)
{
    if (is_a<ConstantI32>(src))
        gen_instr_op2_before(RVENUM::rv_l, new RVReg(ssara->getReg(dst)), new RVImme(((ConstantI32 *)src)->get_32_at(0)), cur_b);
    else if (is_a<ConstantF32>(src))
        gen_instr_op2_before(RVENUM::rv_l, new RVReg(ssara->getReg(dst)), new RVImmef(((ConstantF32 *)src)->get_32_at(0)), cur_b);
    else if (is_a<GlobalVariable>(src))
        gen_load_GV_addr(ssara->getReg(dst), src);
    else
    {
        RVOperand *op1 = get_op(src);
        if (ssara->getReg(dst) == t0 && (is_a<RVReg>(op1) && rv_is_f_reg(((RVReg *)op1)->get_no())))
        {
            gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ft0), op1, cur_b);
        }
        else if (rv_is_f_reg(ssara->getReg(dst)) && (is_a<RVReg>(op1) && (((RVReg *)op1)->get_no() == t0)))
        {
            gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ssara->getReg(dst)), new RVReg(ft0), cur_b);
            delete op1;
        }
        else
            gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ssara->getReg(dst)), op1, cur_b);
    }

    return cur_b->get_last_instr();
}
static inline int posofpa(Function *f, Param *p)
{
    int pos = 0;
    for (auto pa : *f->get_params())
    {
        if (pa == p)
            return pos;
        pos++;
    }
    assert(0);
}

void RVGen::gen_call_before(Instrution *i)
{
    // |stillalive|
    // |(movs)not in stack|
    // |stack param|

    // params to stack
    int off = 0;
    for (auto patostack : infomap.find((Function *)i->get_operand_at(0))->second->paramstostack)
    {
        Value *tostack = i->get_operand_at(posofpa((Function *)i->get_operand_at(0), patostack) + 1);
        tostack->print();
        RVOperand *op1;
        if (is_a<Alloca>(tostack) && ssara->isSpill((Alloca *)tostack))
        {
            assert(((Alloca *)tostack)->get_type()->get_type() == TypeEnum::Ptr);
            Ptr *ptr = (Ptr *)((Alloca *)tostack)->get_type();
            if (ptr->get_btype()->get_type() == TypeEnum::Ptr)
            {
                gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(t0), get_op_addr(tostack), cur_b);
                op1 = new RVReg_d(t0);
            }
            else
            {
                gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), get_op_addr(tostack), cur_b);
                op1 = new RVReg(t0);
            }
        }
        else
            op1 = get_op(tostack);
        gen_instr_op2_before(RVENUM::rv_s, op1, new RVReg_off(sp, off), cur_b);
        off += 8;
    }

    // still alives to stack
    RVInstr *pos = nullptr;
    if (ssara->getFirstMoveofCall((Call *)i))
    {
        auto firstmv = ssara->getFirstMoveofCall((Call *)i);
        assert(val2valmap.find(firstmv) != val2valmap.end());
        pos = (RVInstr *)val2valmap.find(firstmv)->second;
    }
    auto alives = ssara->regsStillAliveAfterCall((Call *)i);
    for (auto alive : alives)
    {
        printf("alives:::%d\n", alive);
        // problem if off is big
        gen_instr_op2_before(RVENUM::rv_s, new RVReg_d(alive), new RVReg_off(sp, off), cur_b, pos);
        off += 8;
    }
    printf("alivesend--\n");
}
void RVGen::gen_call_after(Instrution *i)
{
    if (i->get_type()->get_type() == TypeEnum::Void)
    {
    }
    else if (i->get_type()->get_type() == TypeEnum::I32)
    {
        assert(rv_is_r_reg(ssara->getReg(i)));
        if (ssara->getReg(i) != a0)
            gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ssara->getReg(i)), new RVReg(a0), cur_b);
    }
    else if (i->get_type()->get_type() == TypeEnum::F32)
    {
        assert(rv_is_f_reg(ssara->getReg(i)));
        if (ssara->getReg(i) != fa0)
            gen_instr_op2_before(RVENUM::rv_mv, new RVReg(ssara->getReg(i)), new RVReg(fa0), cur_b);
    }
    else
        assert(0);

    // ldr still alive
    int stackparamsize = 8 * infomap.find((Function *)i->get_operand_at(0))->second->paramstostack.size();
    auto alives = ssara->regsStillAliveAfterCall((Call *)i);
    int off = 0;
    for (auto alive : alives)
    {
        gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(alive), new RVReg_off(sp, stackparamsize + off), cur_b);
        off += 8;
    }
}
void RVGen::gen_call(Instrution *i)
{
    assert(is_a<Function>(i->get_operand_at(0)));
    // call before
    gen_call_before(i);

    auto newi = new RVInstr(RVENUM::rv_call);
    Function *f = (Function *)i->get_operand_at(0);
    newi->ops_push_back(new RVAddr((RVLable *)getrvval(f)));
    cur_b->instr_push_back(newi);

    // call after
    gen_call_after(i);
}