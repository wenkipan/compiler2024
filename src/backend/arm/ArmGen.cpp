#include <cstdint>
#include <cstdio>
#include <iostream>

#include "../../../include/backend/arm/ArmGen.hpp"
#include "../../../include/backend/arm/BlockMerge.hpp"
#include "../../../include/backend/arm/arm.hpp"

static inline void gen_instr_op2(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmBlock *b)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    b->instrs_push_back(newi);
}
static inline void gen_instr_op2_before(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmBlock *b, int pos)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    b->instrs_insert_before(pos, newi);
}
static inline void gen_instr_op3(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmOperand *o3, ArmBlock *b)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    newi->ops_push_back(o3);
    b->instrs_push_back(newi);
}
static inline void gen_instr_op3_before(ARMENUM ae, ArmOperand *o1, ArmOperand *o2, ArmOperand *o3, ArmBlock *b, int pos)
{
    auto newi = new ArmInstr(ae);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    newi->ops_push_back(o3);
    b->instrs_insert_before(pos, newi);
}
void ArmGen::init(Module *m)
{
    arm_module = new ArmModule(m->get_infile(), m->get_outfile());
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
    //  deal params
    std::vector<Param *> paramstostack;
    int iount = 4, fount = 16;
    for (auto pa : *f->get_params())
    {
        if (pa->get_type()->get_type() == TypeEnum::I32 || pa->get_type()->get_type() == TypeEnum::Ptr)
        {
            if (iount)
                iount--;
            else
                paramstostack.push_back(pa);
        }
        else if (pa->get_type()->get_type() == TypeEnum::F32)
        {
            if (fount)
                fount--;
            else
                paramstostack.push_back(pa);
        }
        else
            assert(0);
    }
    printf("---test-paramstostack\n");
    for (auto pa : paramstostack)
    {
        pa->print();
    }
    paraminfomap.emplace(f, paramsinfo(paramstostack));
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
        std::string na;
        na = "." + BB->get_func()->get_name() + "_b" + std::to_string(BB->get_ID());
        ArmBlock *newb = new ArmBlock(na, newf);
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
    printf("--total arm\n");
    arm_module->print(1);

    BlockMerge bm;
    bm.run(arm_module);
    arm_module->print(1);
}
void ArmGen::gen_func(Function *f)
{

    ssara = ssaramap.find(f)->second;
    printf("---------after ssa\n");
    f->print();
    fflush(stdout);
    gen_sp_sub_and_offset_for_alloc_and_param(f, get_ab(f->get_entryBB()));

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
}
static inline bool find_if_call(Function *f)
{
    for (auto B : *f->get_blocks())
        for (auto i : *B->get_instrs())
            if (i->isCall())
                return 1;
    return 0;
}
void ArmGen::gen_sp_sub_and_offset_for_alloc_and_param(Function *f, ArmBlock *b)
{
    // erase before
    alloc_and_param_offset_map.clear();

    // find which reg to push and vpush
    // push { r4 r5 lr r11}
    std::set<int> used_reg;
    if (find_if_call(f))
        used_reg.emplace(LR);

    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
        {
            if (is_a<Move>(instr))
            {
                used_reg.emplace(ssara->getReg(instr->get_operand_at(0)));
                // used_reg.emplace(ssara->getReg(instr->get_operand_at(1)));
            }
            if (ssara->getReg(instr) != -1)
                used_reg.emplace(ssara->getReg(instr));
        }
    printf("usedr");
    for (auto k : used_reg)
        printf("~~%d", k);
    std::vector<int> topushstack;
    for (auto k : used_reg)
    {
        if (k == SP || k == PC)
            assert(0);
        if (k <= 3 || k == 12)
        {
        }
        else if (k <= 15)
            topushstack.push_back(k);
        else if (k <= 15 + 16)
        {
        }
        else if (k <= 15 + 32)
            topushstack.push_back(k);
        else
            assert(0);
    }
    gen_push_or_pop(ARMENUM::arm_push, topushstack, b, b->get_instrs().size());

    int pushedsize = 4 * topushstack.size();

    // total alloc size, max  callee param size
    int max_pushed_callee_param = find_max_pushed_callee_param(f);
    int allocsize = find_all_alloc_size(f);

    // gen_sp_sub
    // sp sub sp_sub_offset
    int sp_sub_offset = allocsize + max_pushed_callee_param;

    if ((sp_sub_offset + pushedsize) % 8) // allgin
        sp_sub_offset += 4;
    printf("ASDDDDDDDDDD%d\n", max_pushed_callee_param);
    printf("ASDDDDDDDDDD%d\n", allocsize);
    printf("ASDDDDDDDDDD%d\n", sp_sub_offset);
    if (sp_sub_offset != 0)
    {
        gen_instr_op3(ARMENUM::arm_sub, new ArmReg(SP), new ArmReg(SP), gen_legal_imme(sp_sub_offset, b), b);
    }

    // gen_sp_offset_for_alloc_and_params

    // build which param push to stack
    // set offset for params
    int papos = 0;
    for (auto pa : paraminfomap.find(f)->second.paramstostack)
    {
        printf("setpaoff:%d  ", pushedsize + sp_sub_offset + 4 * papos);
        pa->print();
        printf("\n");
        set_offset(pa, pushedsize + sp_sub_offset + 4 * (papos++));
    }

    // set offset for allocs
    for (auto BB : *f->get_blocks())
        for (auto instr : *BB->get_instrs())
            if (is_a<Alloca>(instr))
                set_offset(instr, max_pushed_callee_param + get_offset(instr));

    spinfomap.emplace(f, SPinfo(sp_sub_offset, topushstack));
}
void ArmGen::gen_sp_add_and_pop(Function *f, ArmBlock *b)
{
    // gen_sp_sadd
    // sp add sp sp_sub_offset
    int sp_sub_offset = spinfomap.find(f)->second.sp_sub_offset;
    if (sp_sub_offset != 0)
    {
        gen_instr_op3_before(ARMENUM::arm_add, new ArmReg(SP), new ArmReg(SP),
                             gen_legal_imme(sp_sub_offset, b, b->get_instrs().size() - 1), b, b->get_instrs().size() - 1);
    }
    // gen pop
    gen_push_or_pop(ARMENUM::arm_pop, spinfomap.find(f)->second.used_reg, b, b->get_instrs().size() - 1);
}
void ArmGen::gen_mov_imme32(int Rno, int imme, ArmBlock *bb)
{
    printf("to uint32\n");
    printf("%u", (uint32_t)imme);
    uint32_t low = ((1 << 16) - 1) & (uint32_t)imme;
    uint32_t high = ((uint32_t)imme >> 16);
    std::cout << "---imme high";
    std::cout << high << std::endl;
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
void ArmGen::gen_mov_imme32(int Rno, uint32_t imme, ArmBlock *bb)
{
    uint32_t low = 0xFFFF & (uint32_t)imme;
    uint32_t high = ((uint32_t)imme >> 16) & 0xFFFF;
    std::cout << "---imme high";
    std::cout << high << std::endl;
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
void ArmGen::gen_mov_imme32(int Rno, uint32_t imme, ArmBlock *bb, int pos)
{
    uint32_t low = 0xFFFF & (uint32_t)imme;
    uint32_t high = ((uint32_t)imme >> 16) & 0xFFFF;
    std::cout << "---imme high";
    std::cout << high << std::endl;
    ArmInstr *newi = new ArmInstr(ARMENUM::arm_movw);
    newi->ops_push_back(new ArmReg(Rno));
    newi->ops_push_back(new ArmImme(low));
    bb->instrs_insert_before(pos, newi);
    if (high != 0)
    {
        newi = new ArmInstr(ARMENUM::arm_movt);
        newi->ops_push_back(new ArmReg(Rno));
        newi->ops_push_back(new ArmImme(high));
        bb->instrs_insert_before(pos, newi);
    }
    return;
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
ArmInstr *ArmGen::gen_mov(Value *dst, Value *src, ArmBlock *bb)
{
    // int2reg
    if (is_a<ConstantI32>(src))
    {
        gen_mov_imme(ssara->getReg(dst), ((ConstantI32 *)src)->get_32_at(0), bb);
    }
    else if (is_a<ConstantF32>(src))
    {
        // 比较幽默的是，寄存器分配没考虑这个，float的mov用的ldr
        // 比较幽默的是，全部做完之后我发现arm也有mov伪指令，意思是浮点数可以用mov r12 来做      mmp
        std::string na = ".Lconst" + bb->get_parent()->get_name() + std::to_string(bb->get_parent()->get_constlable().size());
        float f = ((ConstantF32 *)src)->get_32_at(0);
        auto constlable = new Armconstlable(na, *(uint32_t *)(&f), bb->get_parent());
        bb->get_parent()->constlable_push_back(constlable);
        gen_instr_op2(ARMENUM::arm_vldr_32, new ArmReg(ssara->getReg(dst)), new ArmAddr(constlable), bb);
    }
    else if (is_a<GlobalVariable>(src))
    {
        gen_load_GV_addr(ssara->getReg(dst), src, bb);
    }
    else if (is_a<Alloca>(src))
    {
        // assert(0);
        gen_instr_op3(ARMENUM::arm_add, new ArmReg(ssara->getReg(dst)), new ArmReg(SP), gen_legal_imme(get_offset(src), bb), bb);
    }
    else if (!is_s_reg(ssara->getReg(dst)) && !is_s_reg(ssara->getReg(src)))
    {
        gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(dst)), get_op(src, bb, 0), bb);
    }
    else
    {
        gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(ssara->getReg(dst)), get_op(src, bb, 0), bb);
    }
    return bb->get_instrs().back();
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
    // do something to alloc
    if (alloc_and_param_offset_map.find(addr) == alloc_and_param_offset_map.end())
    {
        assert(is_a<Alloca>(addr));
        assert(ssara->whichPara((Alloca *)addr));
        assert(alloc_and_param_offset_map.find(ssara->whichPara((Alloca *)addr)) != alloc_and_param_offset_map.end());
        return alloc_and_param_offset_map.find(ssara->whichPara((Alloca *)addr))->second;
    }
    return alloc_and_param_offset_map.find(addr)->second;
}
void ArmGen::set_offset(Value *a, int off)
{
    if (alloc_and_param_offset_map.find(a) != alloc_and_param_offset_map.end())
        alloc_and_param_offset_map.find(a)->second = off;
    else
        alloc_and_param_offset_map.emplace(a, off);
}

int ArmGen::find_max_pushed_callee_param(Function *f)
{
    int max_pushed_callee_param = 0;
    for (auto B : *f->get_blocks())
        for (auto i : *B->get_instrs())
            if (i->isCall())
            {
                assert(ssaramap.find(f) != ssaramap.end());
                assert(paraminfomap.find((Function *)i->get_operand_at(0)) != paraminfomap.end());
                int wqzhemechang = ssaramap.find(f)->second->regsStillAliveAfterCall((Call *)i).size();
                int parmstostack = paraminfomap.find((Function *)i->get_operand_at(0))->second.allcount();
                printf("wqzhemechang:%d\n", wqzhemechang);
                printf("wqzhemechang:%d\n", parmstostack);
                if (max_pushed_callee_param < wqzhemechang + parmstostack)
                    max_pushed_callee_param = wqzhemechang + parmstostack;
            }
    return max_pushed_callee_param * 4;
}
int ArmGen::find_all_alloc_size(Function *f)
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
                else
                    allocsize += 4;
            }
    return allocsize;
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
static inline void gen_str(ArmReg *stored, ArmReg *addr, ArmBlock *b, int pos)
{
    if (stored->is_r_reg())
        gen_instr_op2_before(ARMENUM::arm_str, stored, addr, b, pos);
    else if (stored->is_s_reg())
        gen_instr_op2_before(ARMENUM::arm_vstr_32, stored, addr, b, pos);
    else
        assert(0);
}
static inline void gen_ldr(ArmReg *stored, ArmReg *addr, ArmBlock *b, int pos)
{
    if (stored->is_r_reg())
        gen_instr_op2_before(ARMENUM::arm_ldr, stored, addr, b, pos);
    else if (stored->is_s_reg())
        gen_instr_op2_before(ARMENUM::arm_vldr_32, stored, addr, b, pos);
    else
        assert(0);
}
void ArmGen::gen_call_before(Instrution *i, ArmBlock *b)
{
    // |stillalive|
    // |(movs)not in stack|
    // |stack param|

    // params to stack
    int off = 0;
    for (auto patostack : paraminfomap.find((Function *)i->get_operand_at(0))->second.paramstostack)
    {
        Value *tostack = i->get_operand_at(posofpa((Function *)i->get_operand_at(0), patostack) + 1);
        tostack->print();
        ArmReg *op1;
        if (is_a<Alloca>(tostack) && ssara->isSpill((Alloca *)tostack))
        {
            gen_ldr(new ArmReg(RTMP), (ArmReg *)get_op_addr(tostack, b), b, b->get_instrs().size());
            op1 = new ArmReg(RTMP);
        }
        else
            op1 = (ArmReg *)get_op(tostack, b, 1);
        gen_str(op1, new ArmReg(SP, off), b, b->get_instrs().size());
        off += 4;
    }

    // still alives to stack
    int pos;
    if (ssara->getFirstMoveofCall((Call *)i))
    {
        auto firstmv = ssara->getFirstMoveofCall((Call *)i);
        assert(val2val_map.find(firstmv) != val2val_map.end());
        pos = b->find_instr_pos((ArmInstr *)val2val_map.find(firstmv)->second);
    }
    else
    {
        pos = b->get_instrs().size();
    }
    auto alives = ssara->regsStillAliveAfterCall((Call *)i);
    for (auto alive : alives)
    {
        printf("alives:::%d\n", alive);
        // problem if off is big
        gen_str(new ArmReg(alive), new ArmReg(SP, off), b, pos);
        off += 4;
    }
    printf("alivesend--\n");
}
void ArmGen::gen_call_after(Instrution *i, ArmBlock *b)
{
    if (i->get_type()->get_type() == TypeEnum::Void)
    {
    }
    else if (i->get_type()->get_type() == TypeEnum::I32)
    {
        assert(!is_s_reg(ssara->getReg(i)));
        if (((Function *)i->get_operand_at(0))->get_name() == "__aeabi_idivmod") // special
        {
            if (ssara->getReg(i) != R0 + 1)
                gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(i)), new ArmReg(R0 + 1), b);
        }
        else if (ssara->getReg(i) != R0)
            gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(i)), new ArmReg(R0), b);
    }
    else if (i->get_type()->get_type() == TypeEnum::F32)
    {
        assert(is_s_reg(ssara->getReg(i)));
        if (ssara->getReg(i) != S0)
            gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(ssara->getReg(i)), new ArmReg(S0), b);
    }
    else
        assert(0);

    // ldr still alive
    int stackparamsize = 4 * paraminfomap.find((Function *)i->get_operand_at(0))->second.allcount();
    auto alives = ssara->regsStillAliveAfterCall((Call *)i);
    int off = 0;
    for (auto alive : alives)
    {
        gen_ldr(new ArmReg(alive), new ArmReg(SP, stackparamsize + off), b, b->get_instrs().size());
        off += 4;
    }
}

void ArmGen::gen_call(Instrution *i, ArmBlock *b)
{
    assert(is_a<Function>(i->get_operand_at(0)));
    // call before
    gen_call_before(i, b);

    auto newi = new ArmInstr(ARMENUM::arm_bl);
    Function *f = (Function *)i->get_operand_at(0);
    newi->ops_push_back(new ArmAddr(getArmval(f)));
    b->instrs_push_back(newi);

    // call after
    gen_call_after(i, b);
}
void ArmGen::gen_push_or_pop(ARMENUM ae, std::vector<int> v, ArmBlock *b, int pos)
{
    // insert before
    ARMENUM ae2 = ARMENUM::arm_vpop;
    if (ae == ARMENUM::arm_push)
        ae2 = ARMENUM::arm_vpush;

    ArmInstr *pushs = nullptr;
    ArmInstr *vpushs = nullptr;
    for (auto no : v)
    {
        if (no <= 15)
        {
            if (pushs == nullptr)
            {
                pushs = new ArmInstr(ae);
            }
            pushs->ops_push_back(new ArmReg(no));
        }
        else if (is_s_reg(no))
        {
            if (vpushs == nullptr)
            {
                vpushs = new ArmInstr(ae2);
            }
            vpushs->ops_push_back(new ArmReg(no));
        }
        else
            assert(0);
    }
    if (ae == ARMENUM::arm_push)
    {
        if (vpushs)
            b->instrs_insert_before(pos, vpushs);
        if (pushs)
            b->instrs_insert_before(pos, pushs);
    }
    else if (ae == ARMENUM::arm_pop)
    {
        if (pushs)
            b->instrs_insert_before(pos, pushs);
        if (vpushs)
            b->instrs_insert_before(pos, vpushs);
    }
    else
        assert(0);
}
void ArmGen::gen_ret(Instrution *i, ArmBlock *b)
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
            gen_mov_imme(R0, ((ConstantI32 *)src)->get_32_at(0), b);
        }
        else if (is_a<ConstantF32>(src))
        {
            gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(S0), new ArmImmef(((ConstantF32 *)src)->get_32_at(0)), b);
        }
        else if (is_r_reg(ssara->getReg(src)))
        {
            if (ssara->getReg(src) != R0)
                gen_instr_op2(ARMENUM::arm_mov, new ArmReg(R0), new ArmReg(ssara->getReg(src)), b);
        }
        else if (is_s_reg(ssara->getReg(src)))
        {
            if (ssara->getReg(src) != S0)
                gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(S0), new ArmReg(ssara->getReg(src)), b);
        }
        else
            assert(0);
    }

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
        gen_instr_op2(ARMENUM::arm_vcmp_f32, get_op(cmp->get_operand_at(0), b, 1), get_op(cmp->get_operand_at(1), b, 1), b);
    }
    else if (cmp->get_operand_at(0)->get_type()->get_type() == TypeEnum::I32)
    {
        gen_instr_op2(ARMENUM::arm_cmp, get_op(cmp->get_operand_at(0), b, 1), get_op(cmp->get_operand_at(1), b, 0), b);
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
void ArmGen::gen_load_GV_addr(int no, Value *addr, ArmBlock *b)
{
    assert(is_a<GlobalVariable>(addr));
    auto newi = new ArmInstr(ARMENUM::arm_movw);
    newi->ops_push_back(new ArmReg(no));
    newi->ops_push_back(new ArmAddr(getArmval((GlobalVariable *)addr)));
    b->instrs_push_back(newi);
    newi = new ArmInstr(ARMENUM::arm_movt);
    newi->ops_push_back(new ArmReg(no));
    newi->ops_push_back(new ArmAddr(getArmval((GlobalVariable *)addr)));
    b->instrs_push_back(newi);
}
ArmReg *ArmGen::gen_sp_and_offset_op(int offset, ArmBlock *b)
{
    if (0 <= offset && offset < imm_12_max)
    {
        return new ArmReg(SP, offset);
    }
    else
    {
        gen_mov_imme32(RTMP, offset, b);
        gen_instr_op3(ARMENUM::arm_add, new ArmReg(RTMP), new ArmReg(RTMP), new ArmReg(SP), b);
        return new ArmReg(RTMP, 0);
    }
}
ArmReg *ArmGen::gen_sp_and_offset_op_float(int offset, ArmBlock *b)
{
    printf("manyparams\n");
    printf("offset%d", offset);
    if (-1020 <= offset && offset <= 1020)
    {
        printf("111manyparams\n");
        printf("offsetintheascs%d", offset);

        return new ArmReg(SP, offset);
    }
    else
    {
        printf("manyparamscccc\n");
        gen_mov_imme32(RTMP, offset, b);
        gen_instr_op3(ARMENUM::arm_add, new ArmReg(RTMP), new ArmReg(RTMP), new ArmReg(SP), b);
        return new ArmReg(RTMP, 0);
    }
}
void ArmGen::gen_load(Instrution *i, ArmBlock *b)
{
    Value *addr = i->get_operand_at(0);
    if (ssara->whichGV(dynamic_cast<Alloca *>(addr)))
    {
        gen_load_GV_addr(ssara->getReg(i), ssara->whichGV(dynamic_cast<Alloca *>(addr)), b);
    }
    else if (is_r_reg(ssara->getReg(i)))
    {
        gen_instr_op2(ARMENUM::arm_ldr, new ArmReg(ssara->getReg(i)), get_op_addr(addr, b), b);
    }
    else if (is_s_reg(ssara->getReg(i)))
    {
        gen_instr_op2(ARMENUM::arm_vldr_32, new ArmReg(ssara->getReg(i)), get_op_addr_float(addr, b), b);
    }
    else
        assert(0);
}
void ArmGen::gen_store(Instrution *i, ArmBlock *b)
{
    Value *stored = i->get_operand_at(1);
    Value *addr = i->get_operand_at(0);

    ArmOperand *op1 = nullptr;
    if (is_a<ConstantI32>(stored))
    {
        gen_mov_imme(RTMP, ((ConstantI32 *)stored)->get_32_at(0), b);
        op1 = new ArmReg(RTMP);
    }
    else if (is_a<ConstantF32>(stored))
        assert(0);
    else
    {
        op1 = new ArmReg(ssara->getReg(stored));
    }

    if (((ArmReg *)op1)->is_r_reg())
    {
        gen_instr_op2(ARMENUM::arm_str, op1, get_op_addr(addr, b), b);
    }
    else if (((ArmReg *)op1)->is_s_reg())
    {
        gen_instr_op2(ARMENUM::arm_vstr_32, op1, get_op_addr_float(addr, b), b);
    }
    else
        assert(0);
}
void ArmGen::gen_cmp(Instrution *cmp, ArmBlock *b)
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
        auto newcmp = new ArmInstr(ARMENUM::arm_vcmp_f32);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b, 1));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b, 1));
        b->instrs_push_back(newcmp);
    }
    else if (ty2 == TypeEnum::I32)
    {
        auto newcmp = new ArmInstr(ARMENUM::arm_cmp);
        newcmp->ops_push_back(get_op(cmp->get_operand_at(0), b, 1));
        newcmp->ops_push_back(get_op(cmp->get_operand_at(1), b, 0));
        b->instrs_push_back(newcmp);
    }
    else
        assert(0);

    gen_instr_op2(ARMENUM::arm_mov, new ArmReg(ssara->getReg(cmp)), new ArmImme((uint32_t)0), b);
    ARMENUM ae;
    switch (cmp->get_Instrtype())
    {
    case InstrutionEnum::IEQ:
    case InstrutionEnum::FEQ:
        ae = ARMENUM::arm_moveq;
        break;
    case InstrutionEnum::INEQ:
    case InstrutionEnum::FNEQ:
        ae = ARMENUM::arm_movne;
        break;
    case InstrutionEnum::IGT:
    case InstrutionEnum::FGT:
        ae = ARMENUM::arm_movge;
        break;
    case InstrutionEnum::IGE:
    case InstrutionEnum::FGE:
        ae = ARMENUM::arm_movgt;
        break;
    case InstrutionEnum::ILT:
    case InstrutionEnum::FLT:
        ae = ARMENUM::arm_movlt;
        break;
    case InstrutionEnum::ILE:
    case InstrutionEnum::FLE:
        ae = ARMENUM::arm_movle;
        break;
    default:
        assert(0);
        break;
    }
    gen_instr_op2(ae, new ArmReg(ssara->getReg(cmp)), new ArmImme((uint32_t)1), b);
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
        auto mv = gen_mov(i->get_operand_at(0), i->get_operand_at(1), b);
        val2val_map.emplace(i, mv);
    }
    else
        assert(0);
}
ArmOperand *ArmGen::gen_legal_imme(int imme, ArmBlock *b)
{
    if (imme >= 0)
    {
        if (is_legal_rotate_imme(imme))
            return new ArmImme((uint32_t)imme);
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
ArmOperand *ArmGen::gen_legal_imme(int imme, ArmBlock *b, int pos)
{
    if (imme >= 0)
    {
        if (is_legal_rotate_imme(imme))
            return new ArmImme((uint32_t)imme);
        else
        {
            gen_mov_imme32(RTMP, imme, b, pos);
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
            b->instrs_insert_before(pos, newi);
            return new ArmReg(RTMP);
        }
        else
        {
            gen_mov_imme32(RTMP, imme, b, pos);
            return new ArmReg(RTMP);
        }
    }
}
ArmOperand *ArmGen::get_op_addr(Value *addr, ArmBlock *b)
{
    if (is_a<GlobalVariable>(addr))
    {
        gen_load_GV_addr(ssara->getReg(addr), addr, b);
        return new ArmReg(ssara->getReg(addr), 0);
    }
    else if (is_a<Alloca>(addr))
    {
        // if (ssara->whichGV(dynamic_cast<Alloca *>(addr)))
        // {
        //     gen_load_GV_addr(RTMP, ssara->whichGV(dynamic_cast<Alloca *>(addr)), b);
        //     return new ArmReg(RTMP, 0);
        // }
        return gen_sp_and_offset_op(get_offset(addr), b);
    }
    else // gep results
        return new ArmReg(ssara->getReg(addr), 0);
}
ArmOperand *ArmGen::get_op_addr_float(Value *addr, ArmBlock *b)
{
    if (is_a<GlobalVariable>(addr))
    {
        gen_load_GV_addr(ssara->getReg(addr), addr, b);
        return new ArmReg(ssara->getReg(addr), 0);
    }
    else if (is_a<Alloca>(addr))
    {
        return gen_sp_and_offset_op_float(get_offset(addr), b);
    }
    else // gep results
        return new ArmReg(ssara->getReg(addr), 0);
}
ArmOperand *ArmGen::get_op(Value *i, ArmBlock *b, int must_reg)
{
    if (is_a<ConstantI32>(i))
    {
        if (must_reg)
        {
            gen_mov_imme(RTMP, ((ConstantI32 *)i)->get_32_at(0), b);
            return new ArmReg(RTMP);
        }
        int imme = ((ConstantI32 *)i)->get_32_at(0);
        return gen_legal_imme(imme, b);
    }
    else if (is_a<ConstantF32>(i))
    {
        assert(0);
        float f = ((ConstantF32 *)i)->get_32_at(0);
        gen_mov_imme32(RTMP, *(uint32_t *)(&f), b);
        return new ArmReg(RTMP);
    }
    else if (is_a<Alloca>(i)) // for geps
    {
        gen_instr_op3(ARMENUM::arm_add, new ArmReg(RTMP), new ArmReg(SP), gen_legal_imme(get_offset(i), b), b);
        return new ArmReg(RTMP);
    }
    else if (is_a<GlobalVariable>(i))
    {
        gen_load_GV_addr(ssara->getReg(i), i, b);
        return new ArmReg(ssara->getReg(i));
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
    ArmOperand *dst = get_op(i, b, 0);
    ArmOperand *op1 = get_op(src1, b, 0);
    ArmOperand *op2 = get_op(src2, b, 0);
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
    case InstrutionEnum::ASHR:
        gen_instr_op3(ARMENUM::arm_lsr, dst, op1, op2, b);
        return;
    default:
        break;
    }

    assert(!is_a<ArmImme>(op1) && !is_a<ArmImme>(op2));
    switch (i->get_Instrtype())
    {
    case InstrutionEnum::IMUL:
        // if (is_a<ArmImme>(op1))
        // {
        //     gen_mov_imme(RTMP, ((ConstantI32 *)src1)->get_32_at(0), b);
        //     delete op1;
        //     op1 = new ArmReg(RTMP);
        // }
        // else if (is_a<ArmImme>(op2))
        // {
        //     gen_mov_imme(RTMP, ((ConstantI32 *)src2)->get_32_at(0), b);
        //     delete op2;
        //     op2 = new ArmReg(RTMP);
        // }
        gen_instr_op3(ARMENUM::arm_mul, dst, op1, op2, b);
        return;
    case InstrutionEnum::IDIV:
        // if (is_a<ArmImme>(op1))
        // {
        //     gen_mov_imme(RTMP, ((ConstantI32 *)src1)->get_32_at(0), b);
        //     delete op1;
        //     op1 = new ArmReg(RTMP);
        // }
        // else if (is_a<ArmImme>(op2))
        // {
        //     gen_mov_imme(RTMP, ((ConstantI32 *)src2)->get_32_at(0), b);
        //     delete op2;
        //     op2 = new ArmReg(RTMP);
        // }
        gen_instr_op3(ARMENUM::arm_sdiv, dst, op1, op2, b);
        return;
    case InstrutionEnum::IMOD:
        assert(0);
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
        assert(0);
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
        // VCVT.S32.F32 S1, S1：将S1中的浮点数转换为32位整数，结果存入单精度浮点寄存器S1。
        // VMOV R0, S1：将转换后的整数值从浮点寄存器S1传送到常规寄存器R0。
        // 恢复(哥们儿看了一个小时bug终于看出来了)
        gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(RTMP), new ArmReg(ssara->getReg(src1)), b);
        gen_instr_op2(ARMENUM::arm_vcvt_s32_f32, new ArmReg(ssara->getReg(src1)), new ArmReg(ssara->getReg(src1)), b);
        gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(ssara->getReg(i)), new ArmReg(ssara->getReg(src1)), b);
        gen_instr_op2(ARMENUM::arm_vmov, new ArmReg(ssara->getReg(src1)), new ArmReg(RTMP), b);
        break;
    case InstrutionEnum::I2F:
        // VMOV S1, R1：将R1中的整数值传送到单精度浮点寄存器S1。
        // VCVT.F32.S32 S0, S1：将S1中的整数值转换为单精度浮点数，结果存入单精度浮点寄存器S0。
        newi = new ArmInstr(ARMENUM::arm_vmov);
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