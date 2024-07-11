#include "ir/Type.hpp"
#include <backend/arm/ArmGen.hpp>
#include <cstdio>
void ArmGen::init(Module *m)
{
    arm_module = new ArmModule;
    for (auto f : *m->get_funcs())
    {
        auto newf = init_func(f);
        arm_module->funcs_push_back(newf);
    }
    // global vals
    for (auto GV : *m->get_globals())
    {
        assert(GV->get_type()->get_type() == TypeEnum::Ptr);
        Type *pointeetype = ((Ptr *)GV->get_type())->get_btype();
        ArmGlobalVariable *newgv = new ArmGlobalVariable(GV->get_name());
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
            else
            {
                // dangerous
                newgv->set_space(4);
            }
        }
        else if (pointeetype->get_type() == TypeEnum::Array)
        {
            newgv->set_space(((ArrayType *)pointeetype)->get_size());
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
        arm_module->gvs_push_back(newgv);
        val2val_map.emplace(GV, newgv);
    }
}
ArmFunc *ArmGen::init_func(Function *f)
{
    auto newf = new ArmFunc();
    newf->set_name(f->get_name());
    // no params yet
    val2val_map.emplace(f, newf);
    if (f->get_isExternal())
    {
        newf->set_external();
        return newf;
    }

    // do ssara
    SSARegisterAlloc *s = new SSARegisterAlloc;
    s->run(f);
    ssaramap.emplace(f, s);

    for (auto BB : *f->get_blocks())
    {
        ArmBlock *newb = new ArmBlock;
        std::string na;
        na = "." + BB->get_func()->get_name() + "_b" + std::to_string(BB->get_ID());
        newb->set_name(na);
        val2val_map.emplace(BB, newb);
        bb_ab_map.emplace(BB, newb);
        newf->blocks_push_back(newb);
    }
    // build edge
    for (auto BB : *f->get_blocks())
    {
        for (auto succ : successors(BB))
        {
            new ArmEdge(getArmval(BB), getArmval(succ));
        }
    }

    return newf;
}
void ArmGen::run(Module *m)
{
    init(m);
    for (auto f : *m->get_funcs())
    {
        if (f->get_isExternal())
            continue;
        gen_func(f);
    }
}
void ArmGen::gen_func(Function *f)
{

    ssara = ssaramap.find(f)->second;
    printf("---------after ssa\n");
    f->print();
    fflush(stdout);
    gen_sp_sub_and_offset_for_alloc_and_param(f, get_ab(f->get_entryBB()));
    printf("--------sp_sub---------\n");
    ((ArmFunc *)val2val_map.find(f)->second)->print();
    printf("--------sp_sub_end---------\n");

    for (auto BB : *f->get_blocks())
    {
        printf("-----BBprint\n");
        BB->print();
        printf("-------GENBLOCKBEGIN--------\n");
        for (auto instr : *BB->get_instrs())
        {
            gen_instr(instr, get_ab(BB));
        }
        printf("-------GENBLOCKend-----\n");
    }
    // pop { r4,r5,lr,r11}
    gen_sp_add_and_pop(f, get_ab(f->get_retBB()));

    printf("--------total arm---------\n");
    ((ArmFunc *)val2val_map.find(f)->second)->print();
    printf("--------total arm end---------\n");
}
static inline bool is_s_reg(int no) { return no > 15; }
void ArmGen::gen_mov_imme32(int Rno, int imme, ArmBlock *bb)
{
    uint32_t low = ((1 << 16) - 1) & imme;
    uint32_t high = (imme >> 16);
    ArmInstr *newi = new ArmInstr(ARMENUM::arm_movw);
    newi->ops_push_back(new ArmReg(Rno));
    newi->ops_push_back(new ArmImme(low));
    bb->instrs_push_back(newi);
    if (high != 0)
    {
        newi = new ArmInstr(ARMENUM::arm_movt);
        newi->ops_push_back(new ArmReg(Rno));
        newi->ops_push_back(new ArmImme(high));
        bb->instrs_push_back(newi);
    }
    return;
}
static inline void gen_instr_op2(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmBlock *b)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    b->instrs_push_back(newi);
}
static inline void gen_instr_op3(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmOperand *o3, ArmBlock *b)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    newi->ops_push_back(o3);
    b->instrs_push_back(newi);
}
void ArmGen::gen_mov_imme(int dst, int imme, ArmBlock *b)
{
    if (imme >= 0)
    {
        if (is_legal_rotate_imme(imme))
        {
            ArmInstr *newi = new ArmInstr(ARMENUM::arm_mov);
            newi->ops_push_back(new ArmReg(dst));
            newi->ops_push_back(new ArmImme((uint32_t)imme));
            b->instrs_push_back(newi);
        }
        else
        {
            gen_mov_imme32(dst, imme, b);
        }
    }
    else
    {
        int neg = -imme - 1;
        if (is_legal_rotate_imme(neg))
        {
            ArmInstr *newim = new ArmInstr(ARMENUM::arm_mvn);
            newim->ops_push_back(new ArmReg(dst));
            newim->ops_push_back(new ArmImme((uint32_t)neg));
            b->instrs_push_back(newim);
        }
        else
        {
            gen_mov_imme32(dst, imme, b);
        }
    }
}
void ArmGen::gen_mov(Value *dst, Value *src, ArmBlock *bb)
{
    // int2reg
    if (is_a<ConstantI32>(src))
    {
        gen_mov_imme(ssara->getReg(dst), ((ConstantI32 *)src)->get_32_at(0), bb);
    }
    else if (is_a<ConstantF32>(src))
    {
        // 比较幽默的是，寄存器分配没考虑这个，float的mov用的ldr
        assert(0);
    }
    else if (!is_s_reg(ssara->getReg(dst)) && !is_s_reg(ssara->getReg(src)))
    {
        gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(dst)), new ArmReg(ssara->getReg(src)), bb);
    }
    else
    {
        gen_instr_op2(ARMENUM::arm_vmov_f32, new ArmReg(ssara->getReg(dst)), new ArmReg(ssara->getReg(src)), bb);
    }
}

ArmBlock *ArmGen::get_ab(BasicBlock *b)
{
    assert(bb_ab_map.find(b) != bb_ab_map.end());
    return bb_ab_map.find(b)->second;
}
ArmValue *ArmGen::getArmval(Value *v)
{
    assert(val2val_map.find(v) != val2val_map.end());
    return val2val_map.find(v)->second;
}
int ArmGen::get_offset(Value *addr)
{
    assert(alloc_and_param_offset_map.find(addr) != alloc_and_param_offset_map.end());
    return alloc_and_param_offset_map.find(addr)->second;
}
void ArmGen::set_offset(Value *a, int off)
{
    if (alloc_and_param_offset_map.find(a) != alloc_and_param_offset_map.end())
        alloc_and_param_offset_map.find(a)->second = off;
    else
        alloc_and_param_offset_map.emplace(a, off);
}

void ArmGen::gen_sp_sub_and_offset_for_alloc_and_param(Function *f, ArmBlock *b)
{
    // erase before
    alloc_and_param_offset_map.clear();

    // find which reg to push and vpush
    // push { r4 r5 lr r11}
    std::set<int> used_reg;
    int pushedsize = 0;
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
        {
            if (!is_a<Alloca>(instr))
                used_reg.emplace(ssara->getReg(instr));
        }
    ArmInstr *newpush = nullptr;
    ArmInstr *newvpush = nullptr;
    std::vector<int> r;
    std::vector<int> s;
    for (auto k : used_reg)
    {
        if (k == SP || k == PC)
            assert(0);
        if (k <= 3 || k == 12)
        {
        }
        else if (k <= 15)
        {
            if (newpush == nullptr)
                newpush = new ArmInstr(ARMENUM::arm_push);
            newpush->ops_push_back(new ArmReg(k));
            r.push_back(k);
            pushedsize += 4;
        }
        else if (k <= 15 + 16)
        {
        }
        else if (k <= 15 + 32)
        {
            if (newpush == nullptr)
                newvpush = new ArmInstr(ARMENUM::arm_vpush);
            newvpush->ops_push_back(new ArmReg(k));
            s.push_back(k);
            pushedsize += 4;
        }
        else
            assert(0);
    }
    if (newpush)
        b->instrs_push_back(newpush);
    if (newvpush)
        b->instrs_push_back(newvpush);
    // total alloc size, max  callee param size
    int max_callee_param_sub4 = 0;
    int allocsize = 0;
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
        {
            if (is_a<Call>(instr))
            {
                if ((int)(instr->get_value_list()->size() - 5) > max_callee_param_sub4)
                {
                    max_callee_param_sub4 = instr->get_value_list()->size() - 5; // one is callee ptr
                    printf("ASDDDDDDDDDD%d\n", max_callee_param_sub4);
                }
            }
            else if (is_a<Alloca>(instr) && ssara->whichPara((Alloca *)instr) == nullptr)
            {
                printf("----alloc----");
                instr->print();
                set_offset(instr, allocsize);
                ((Alloca *)instr)->get_type()->print();
                assert(((Alloca *)instr)->get_type()->get_type() == TypeEnum::Ptr);
                Ptr *ptr = (Ptr *)((Alloca *)instr)->get_type();
                if (ptr->get_btype()->isArray())
                    allocsize += 4 * ((ArrayType *)ptr->get_btype())->get_size();
                else
                    allocsize += 4;
            }
        }
    max_callee_param_sub4 *= 4;

    // gen_sp_sub
    // sp sub sp_sub_offset
    int sp_sub_offset = allocsize + max_callee_param_sub4;
    auto newsubsp = new ArmInstr(ARMENUM::arm_sub);
    newsubsp->ops_push_back(new ArmReg(SP));
    if (is_legal_rotate_imme(sp_sub_offset))
    {
        newsubsp->ops_push_back(new ArmReg(SP));
        printf("ASDDDDDDDDDD%d\n", max_callee_param_sub4);
        printf("ASDDDDDDDDDD%d\n", allocsize);
        printf("ASDDDDDDDDDD%d\n", sp_sub_offset);
        newsubsp->ops_push_back(new ArmImme((uint32_t)sp_sub_offset));
    }
    else
    {
        gen_mov_imme32(RTMP, sp_sub_offset, b);
        newsubsp->ops_push_back(new ArmReg(RTMP));
    }
    b->instrs_push_back(newsubsp);

    // gen_sp_offset_for_alloc_and_params

    // int paramssub4 = f->get_params()->size() - 4;
    int paramscnt = 0;
    for (auto pa : *f->get_params())
    {
        if (paramscnt > 3)
        {
            set_offset(pa, pushedsize + sp_sub_offset + 4 * (paramscnt - 4));
        }
        paramscnt++;
    }
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
            if (is_a<Alloca>(instr))
            {
                Param *k = ssara->whichPara((Alloca *)instr);
                if (k)
                {
                    set_offset(instr, get_offset(k));
                }
                else
                {
                    // int posofalloc = 0;
                    set_offset(instr, max_callee_param_sub4 + get_offset(instr));
                }
            }
    spinfomap.emplace(f, SPinfo(sp_sub_offset, r, s));
}
void ArmGen::gen_sp_add_and_pop(Function *f, ArmBlock *b)
{
    // gen_sp_sadd
    // sp add sp sp_sub_offset
    int sp_sub_offset = spinfomap.find(f)->second.sp_sub_offset;
    auto newsubsp = new ArmInstr(ARMENUM::arm_add);
    newsubsp->ops_push_back(new ArmReg(SP));
    if (is_legal_rotate_imme(sp_sub_offset))
    {
        newsubsp->ops_push_back(new ArmReg(SP));
        newsubsp->ops_push_back(new ArmImme((uint32_t)sp_sub_offset));
    }
    else
    {
        gen_mov_imme32(RTMP, sp_sub_offset, b);
        newsubsp->ops_push_back(new ArmReg(RTMP));
    }
    b->instrs_insert_before(b->get_instrs().size() - 1, newsubsp);
    ArmInstr *newpush = nullptr;
    ArmInstr *newvpush = nullptr;
    for (auto k : spinfomap.find(f)->second.used_s_reg)
    {
        if (newvpush == nullptr)
            newvpush = new ArmInstr(ARMENUM::arm_vpop);
        newvpush->ops_push_back(new ArmReg(k));
    }
    if (newvpush)
        b->instrs_insert_before(b->get_instrs().size() - 1, newvpush);
    for (auto k : spinfomap.find(f)->second.used_r_reg)
    {
        if (newpush == nullptr)
            newpush = new ArmInstr(ARMENUM::arm_pop);
        newpush->ops_push_back(new ArmReg(k));
    }
    if (newpush)
        b->instrs_insert_before(b->get_instrs().size() - 1, newpush);
}

void ArmGen::gen_call(Instrution *i, ArmBlock *b)
{
    for (int pos = 5; pos < i->get_value_list()->size(); pos++)
    {
        ArmInstr *newstr;
        int regno = ssara->getReg(i->get_operand_at(pos));
        if (regno != -1)
        {
            if (is_s_reg(regno))
                newstr = new ArmInstr(ARMENUM::arm_vstr_32);
            else
                newstr = new ArmInstr(ARMENUM::arm_str);
            newstr->ops_push_back(new ArmReg(regno));
            newstr->ops_push_back(new ArmReg(SP, 4 * (pos - 5)));
        }
        else
        {
            newstr = new ArmInstr(ARMENUM::arm_str);
            if (is_a<Alloca>(i->get_operand_at(pos)))
            {
                gen_instr_op2(ARMENUM::arm_ldr, new ArmReg(RTMP), new ArmReg(SP, get_offset(i->get_operand_at(pos))), b);
                newstr->ops_push_back(new ArmReg(RTMP));
            }
            else if (is_a<ConstantI32>(i->get_operand_at(pos)))
            {
                gen_mov_imme(RTMP, ((ConstantI32 *)i->get_operand_at(pos))->get_32_at(0), b);
                newstr->ops_push_back(new ArmReg(RTMP));
            }
            else if (is_a<GlobalVariable>(i->get_operand_at(pos)))
            {
                gen_load_GV_addr(i->get_operand_at(pos), b);
                newstr->ops_push_back(new ArmReg(RTMP));
            }
            else
                assert(0);
            newstr->ops_push_back(new ArmReg(SP, 4 * (pos - 5)));
            //!!!
        }
        b->instrs_push_back(newstr);
    }
    assert(is_a<Function>(i->get_operand_at(0)));
    auto newi = new ArmInstr(ARMENUM::arm_bl);
    Function *f = (Function *)i->get_operand_at(0);
    newi->ops_push_back(new ArmAddr(getArmval(f)));
    b->instrs_push_back(newi);
}
void ArmGen::gen_ret(Instrution *i, ArmBlock *b)
{
    auto newi = new ArmInstr(ARMENUM::arm_bx);
    newi->ops_push_back(new ArmReg(14));
    b->instrs_push_back(newi);
}
void ArmGen::gen_branch(Instrution *i, ArmBlock *b)
{
    BasicBlock *parent = i->get_parent();
    Cmp *cmp = (Cmp *)i->get_operand_at(0);
    // gen cmp
    assert(cmp->get_operand_at(0)->get_type()->get_type() == cmp->get_operand_at(1)->get_type()->get_type());
    if (cmp->get_operand_at(0)->get_type()->get_type() == TypeEnum::F32)
    {
        auto newcmp = new ArmInstr(ARMENUM::arm_vcmp_f32);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b));
        b->instrs_push_back(newcmp);
    }
    else if (cmp->get_operand_at(0)->get_type()->get_type() == TypeEnum::I32)
    {
        auto newcmp = new ArmInstr(ARMENUM::arm_cmp);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b));
        b->instrs_push_back(newcmp);
    }
    else
        assert(0);
    // gen b1
    BasicBlock *truebb = (BasicBlock *)parent->get_user_list()->at(0)->get_user();
    BasicBlock *falsebb = (BasicBlock *)parent->get_user_list()->at(1)->get_user();
    ArmInstr *newi;
    switch (cmp->get_Instrtype())
    {
    case InstrutionEnum::IEQ:
    case InstrutionEnum::FEQ:
        newi = new ArmInstr(ARMENUM::arm_beq);
        break;
    case InstrutionEnum::INEQ:
    case InstrutionEnum::FNEQ:
        newi = new ArmInstr(ARMENUM::arm_bne);
        break;
    case InstrutionEnum::IGT:
    case InstrutionEnum::FGT:
        newi = new ArmInstr(ARMENUM::arm_bgt);
        break;
    case InstrutionEnum::IGE:
    case InstrutionEnum::FGE:
        newi = new ArmInstr(ARMENUM::arm_bge);
        break;
    case InstrutionEnum::ILT:
    case InstrutionEnum::FLT:
        newi = new ArmInstr(ARMENUM::arm_blt);
        break;
    case InstrutionEnum::ILE:
    case InstrutionEnum::FLE:
        newi = new ArmInstr(ARMENUM::arm_ble);
        break;
    default:
        assert(0);
        break;
    }
    newi->ops_push_back(new ArmAddr(get_ab(truebb)));
    b->instrs_push_back(newi);
    // gen b2
    newi = new ArmInstr(ARMENUM::arm_b);
    newi->ops_push_back(new ArmAddr(get_ab(falsebb)));
    b->instrs_push_back(newi);
}
void ArmGen::gen_jmp(Instrution *i, ArmBlock *b)
{
    BasicBlock *parent = i->get_parent();
    BasicBlock *thenbb = (BasicBlock *)parent->get_user_list()->at(0)->get_user();
    auto newi = new ArmInstr(ARMENUM::arm_b);
    newi->ops_push_back(new ArmAddr(getArmval(thenbb)));
    b->instrs_push_back(newi);
}
void ArmGen::gen_load_GV_addr(Value *addr, ArmBlock *b)
{
    assert(is_a<GlobalVariable>(addr));
    // RTMP= addr
    auto newi = new ArmInstr(ARMENUM::arm_movw);
    newi->ops_push_back(new ArmReg(RTMP));
    newi->ops_push_back(new ArmAddr(getArmval((GlobalVariable *)addr)));
    b->instrs_push_back(newi);
    newi = new ArmInstr(ARMENUM::arm_movt);
    newi->ops_push_back(new ArmReg(RTMP));
    newi->ops_push_back(new ArmAddr(getArmval((GlobalVariable *)addr)));
    b->instrs_push_back(newi);
}
ArmReg *ArmGen::gen_sp_and_offset_op(int offset, ArmBlock *b)
{
    if (offset < imm_12_max)
    {
        return new ArmReg(SP, (uint32_t)offset);
    }
    else
    {
        gen_mov_imme32(RTMP, offset, b);
        return new ArmReg(RTMP);
    }
}
void ArmGen::gen_load(Instrution *i, ArmBlock *b)
{
    Value *addr = i->get_operand_at(0);
    if (is_a<GlobalVariable>(addr))
    {
        gen_load_GV_addr(addr, b);
        // R/s=[RTMP]
        ArmInstr *newi;
        if (i->get_type()->get_type() == TypeEnum::I32)
        {
            newi = new ArmInstr(ARMENUM::arm_ldr);
        }
        else if (i->get_type()->get_type() == TypeEnum::F32)
        {
            newi = new ArmInstr(ARMENUM::arm_vldr_32);
        }
        else
            assert(0);
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        newi->ops_push_back(new ArmReg(RTMP));
        b->instrs_push_back(newi);
    }
    else // gep results and alloc
    {
        ArmInstr *newi;
        if (i->get_type()->get_type() == TypeEnum::I32)
        {
            newi = new ArmInstr(ARMENUM::arm_ldr);
        }
        else if (i->get_type()->get_type() == TypeEnum::Ptr)
            newi = new ArmInstr(ARMENUM::arm_ldr);
        else if (i->get_type()->get_type() == TypeEnum::F32)
        {
            newi = new ArmInstr(ARMENUM::arm_vldr_32);
        }
        else
            assert(0);
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        if (is_a<Alloca>(addr))
            newi->ops_push_back(gen_sp_and_offset_op(get_offset(addr), b));
        else // geps
            newi->ops_push_back(new ArmReg(ssara->getReg(addr), 0));
        b->instrs_push_back(newi);
    }
}
void ArmGen::gen_store(Instrution *i, ArmBlock *b)
{
    Value *stored = i->get_operand_at(1);
    Value *addr = i->get_operand_at(0);
    ArmInstr *newi = nullptr;
    if (stored->get_type()->get_type() == TypeEnum::F32)
    {
        newi = new ArmInstr(ARMENUM::arm_vstr_32);
    }
    else if (stored->get_type()->get_type() == TypeEnum::I32)
    {
        newi = new ArmInstr(ARMENUM::arm_str);
    }
    else if (stored->get_type()->get_type() == TypeEnum::Ptr)
    {
        newi = new ArmInstr(ARMENUM::arm_str);
    }
    else
        assert(0);

    if (is_a<ConstantI32>(stored))
    {
        gen_mov_imme(RTMP, ((ConstantI32 *)stored)->get_32_at(0), b);
        newi->ops_push_back(new ArmReg(RTMP));
    }
    else if (is_a<ConstantF32>(stored))
        assert(0);
    else
        newi->ops_push_back(new ArmReg(ssara->getReg(stored)));
    if (is_a<GlobalVariable>(addr))
    {
        gen_load_GV_addr(addr, b);
        // addr
        newi->ops_push_back(new ArmReg(RTMP));
    }
    else if (is_a<Alloca>(addr)) // not sure depends on geps
    {
        newi->ops_push_back(gen_sp_and_offset_op(get_offset(addr), b));
    }
    else
    {
        // gep results,not sure
        newi->ops_push_back(new ArmReg(ssara->getReg(addr)));
    }
    b->instrs_push_back(newi);
}
void ArmGen::gen_cmp(Instrution *cmp, ArmBlock *b)
{
    printf("-----cmptype\n");
    TypeEnum ty1 = cmp->get_operand_at(0)->get_type()->get_type();
    TypeEnum ty2 = cmp->get_operand_at(1)->get_type()->get_type();
    cmp->get_operand_at(1)->get_type()->print();
    fflush(stdout);
    if (ty1 == TypeEnum::I1)
        ty1 = TypeEnum::I32;
    if (ty2 == TypeEnum::I1)
        ty2 = TypeEnum::I32;
    assert(ty1 = ty2);
    if (ty1 == TypeEnum::F32)
    {
        auto newcmp = new ArmInstr(ARMENUM::arm_vcmp_f32);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b));
        b->instrs_push_back(newcmp);
    }
    else if (ty2 == TypeEnum::I32)
    {
        auto newcmp = new ArmInstr(ARMENUM::arm_cmp);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b));
        b->instrs_push_back(newcmp);
    }
    else
        assert(0);

    gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(cmp)), new ArmImme(0), b);
    switch (cmp->get_Instrtype())
    {
    case InstrutionEnum::IEQ:
    case InstrutionEnum::FEQ:
        gen_instr_op2(ARMENUM::arm_moveq, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    case InstrutionEnum::INEQ:
    case InstrutionEnum::FNEQ:
        gen_instr_op2(ARMENUM::arm_movne, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    case InstrutionEnum::IGT:
    case InstrutionEnum::FGT:
        gen_instr_op2(ARMENUM::arm_movge, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    case InstrutionEnum::IGE:
    case InstrutionEnum::FGE:
        gen_instr_op2(ARMENUM::arm_movgt, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    case InstrutionEnum::ILT:
    case InstrutionEnum::FLT:
        gen_instr_op2(ARMENUM::arm_movlt, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    case InstrutionEnum::ILE:
    case InstrutionEnum::FLE:
        gen_instr_op2(ARMENUM::arm_movle, new ArmReg(ssara->getReg(cmp)), new ArmImme(1), b);
        break;
    default:
        assert(0);
        break;
    }
}
void ArmGen::gen_instr(Instrution *i, ArmBlock *b)
{
    printf("GENINSTR::");
    i->print();
    if (i->isAlloca())
    {
        return;
    }
    else if (i->isBinary())
    {
        gen_binary(i, b);
    }
    else if (i->isUnary())
        gen_unary(i, b);
    else if (i->isCall())
    {
        gen_call(i, b);
    }
    else if (i->isReturn())
    {
        gen_ret(i, b);
    }
    else if (i->isBranch())
    {
        gen_branch(i, b);
    }
    else if (i->isJmp())
    {
        gen_jmp(i, b);
    }
    else if (i->isCmp())
    {
        if (((Cmp *)i)->isCond())
            return;
        gen_cmp(i, b);
    }
    else if (i->isLoad())
    {
        gen_load(i, b);
    }
    else if (i->isStore())
    {
        gen_store(i, b);
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
        gen_mov(i->get_operand_at(0), i->get_operand_at(1), b);
    }
    else
        assert(0);
}
ArmOperand *ArmGen::gen_legal_imme(int imme, ArmBlock *b)
{
    if (imme >= 0)
    {
        if (is_legal_rotate_imme(imme))
            return new ArmImme(imme);
        else
        {
            gen_mov_imme32(RTMP, imme, b);
            return new ArmReg(RTMP);
        }
    }
    else
    {
        int neg = -imme - 1;
        if (is_legal_rotate_imme(neg))
        {
            ArmInstr *newi = new ArmInstr(ARMENUM::arm_mvn);
            newi->ops_push_back(new ArmReg(RTMP));
            newi->ops_push_back(new ArmImme((uint32_t)neg));
            b->instrs_push_back(newi);
            return new ArmReg(RTMP);
        }
        else
        {
            gen_mov_imme32(RTMP, imme, b);
            return new ArmReg(RTMP);
        }
    }
}
ArmOperand *ArmGen::get_op(Value *i, ArmBlock *b)
{
    if (is_a<ConstantI32>(i))
    {
        int imme = ((ConstantI32 *)i)->get_32_at(0);
        return gen_legal_imme(imme, b);
    }
    else if (is_a<ConstantF32>(i))
        assert(0);
    else if (is_a<Alloca>(i)) // for geps
    {
        gen_instr_op3(ARMENUM::arm_add, new ArmReg(RTMP), new ArmReg(SP), gen_legal_imme(get_offset(i), b), b);
        return new ArmReg(RTMP);
    }
    else if (is_a<GlobalVariable>(i))
    {
        gen_load_GV_addr(i, b);
        return new ArmReg(RTMP);
    }
    else
    {
        if (i->get_type()->get_type() == TypeEnum::F32)
            assert(is_s_reg(ssara->getReg(i)));
        return new ArmReg(ssara->getReg(i));
    }
}
void ArmGen::gen_binary(Instrution *i, ArmBlock *b)
{
    assert(i->isBinary());

    Value *src1 = i->get_operand_at(0);
    Value *src2 = i->get_operand_at(1);
    ArmOperand *dst = get_op(i, b);
    ArmOperand *op1 = get_op(src1, b);
    ArmOperand *op2 = get_op(src2, b);
    assert(!(is_a<ArmImme>(op1) && is_a<ArmImme>(op2)));
    switch (i->get_Instrtype())
    {
    case InstrutionEnum::IADD:
        if (is_a<ArmImme>(op1))
            std::swap(op1, op2);
        gen_instr_op3(ARMENUM::arm_add, dst, op1, op2, b);
        return;
    case InstrutionEnum::ISUB:
        if (is_a<ArmImme>(op1))
            gen_instr_op3(ARMENUM::arm_rsb, dst, op2, op1, b);
        else
            gen_instr_op3(ARMENUM::arm_sub, dst, op1, op2, b);
        return;
    case InstrutionEnum::SHL:
        gen_instr_op3(ARMENUM::arm_lsl, dst, op1, op2, b);
        return;
    case InstrutionEnum::LSHR:
        gen_instr_op3(ARMENUM::arm_lsr, dst, op1, op2, b);
        return;
    case InstrutionEnum::IMUL:
        if (is_a<ArmImme>(op1))
        {
            gen_mov_imme(RTMP, ((ConstantI32 *)src1)->get_32_at(0), b);
            delete op1;
            op1 = new ArmReg(RTMP);
        }
        else if (is_a<ArmImme>(op2))
        {
            gen_mov_imme(RTMP, ((ConstantI32 *)src2)->get_32_at(0), b);
            delete op2;
            op2 = new ArmReg(RTMP);
        }
        gen_instr_op3(ARMENUM::arm_mul, dst, op1, op2, b);
        return;
    case InstrutionEnum::IDIV:
        if (is_a<ArmImme>(op1))
        {
            gen_mov_imme(RTMP, ((ConstantI32 *)src1)->get_32_at(0), b);
            delete op1;
            op1 = new ArmReg(RTMP);
        }
        else if (is_a<ArmImme>(op2))
        {
            gen_mov_imme(RTMP, ((ConstantI32 *)src2)->get_32_at(0), b);
            delete op2;
            op2 = new ArmReg(RTMP);
        }
        gen_instr_op3(ARMENUM::arm_sdiv, dst, op1, op2, b);
        return;
    case InstrutionEnum::IMOD:
        assert(0);
    default:
        break;
    }

    assert(!is_a<ArmImme>(op1) && !is_a<ArmImme>(op2));
    switch (i->get_Instrtype())
    {
    case InstrutionEnum::FADD:
        gen_instr_op3(ARMENUM::arm_vadd_f32, dst, op1, op2, b);
        break;
    case InstrutionEnum::FSUB:
        gen_instr_op3(ARMENUM::arm_vsub_f32, dst, op1, op2, b);
        break;
    case InstrutionEnum::FMUL:
        gen_instr_op3(ARMENUM::arm_vmul_f32, dst, op1, op2, b);
        break;
    case InstrutionEnum::FDIV:
        gen_instr_op3(ARMENUM::arm_vdiv_f32, dst, op1, op2, b);
        break;
    default:
        break;
    }
}

void ArmGen::gen_unary(Instrution *i, ArmBlock *b)
{
    assert(i->isUnary());
    Value *src1 = i->get_operand_at(0);
    ArmInstr *newi;
    switch (i->get_Instrtype())
    {
    case InstrutionEnum::Assign:
        gen_mov(i, src1, b);
        break;
    case InstrutionEnum::MINUS:
        assert(!is_a<Constant>(src1));
        if (src1->get_type()->get_type() == TypeEnum::F32)
        {
            auto newi = new ArmInstr(ARMENUM::arm_vneg_f32);
            newi->ops_push_back(new ArmReg(ssara->getReg(i)));
            newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
            b->instrs_push_back(newi);
        }
        else if (src1->get_type()->get_type() == TypeEnum::I32)
        {
            auto newi = new ArmInstr(ARMENUM::arm_rsb);
            newi->ops_push_back(new ArmReg(ssara->getReg(i)));
            newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
            newi->ops_push_back(new ArmImme((uint32_t)0));
            b->instrs_push_back(newi);
        }
        break;
    case InstrutionEnum::F2I:
        assert(!is_s_reg(ssara->getReg(i)));
        assert(is_s_reg(ssara->getReg(src1)));
        // VCVT.S32.F32 S1, S0：将S0中的浮点数转换为32位整数，结果存入单精度浮点寄存器S1。
        // VMOV R0, S1：将转换后的整数值从浮点寄存器S1传送到常规寄存器R0。
        newi = new ArmInstr(ARMENUM::arm_vcvt_s32_f32);
        newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
        newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
        b->instrs_push_back(newi);
        newi = new ArmInstr(ARMENUM::arm_vmov_f32);
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
        b->instrs_push_back(newi);
        break;
    case InstrutionEnum::I2F:
        // VMOV S1, R1：将R1中的整数值传送到单精度浮点寄存器S1。
        // VCVT.F32.S32 S0, S1：将S1中的整数值转换为单精度浮点数，结果存入单精度浮点寄存器S0。
        newi = new ArmInstr(ARMENUM::arm_vmov_f32);
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        newi->ops_push_back(new ArmReg(ssara->getReg(src1)));
        b->instrs_push_back(newi);
        newi = new ArmInstr(ARMENUM::arm_vcvt_f32_s32);
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        newi->ops_push_back(new ArmReg(ssara->getReg(i)));
        b->instrs_push_back(newi);
        break;
    default:
        assert(0);
        break;
    }
}