#include <ir_opt/ConstFold.hpp>

Value *const_fold(Instrution *instr)
{
    if (instr->isBinary())
    {
        return const_fold_binary((Binary *)instr);
    }
    else if (instr->isUnary())
    {
        return const_fold_unary((Unary *)instr);
    }
    else if (instr->isCmp())
    {
        return const_fold_Cmp((Cmp *)instr);
    }
    else if (instr->isGEP())
    { // GEP can be fold but our GEP seems need loop analysis
        return nullptr;
    }
    else if (instr->isCall())
    { // TODO: call with all-const can be folded
        return nullptr;
    }
    else if (instr->isLoad())
    { // TODO: wait memryssa
        return nullptr;
    }
    else if (instr->isBranch())
    { // already done in SCCP,maybe can do this here
        return nullptr;
    }
    else if (instr->isAlloca() || instr->isStore() || instr->isReturn() || instr->isJmp())
    {
        return nullptr;
    }
    else
        assert(0);
}
Value *const_fold_unary(Unary *instr)
{
    Value *a = nullptr;
    Value *src = instr->get_src();
    if (is_a<ConstantI32>(src) && instr->get_Instrtype() == InstrutionEnum::I2F)
    {
        a = new ConstantF32(((ConstantI32 *)src)->get_i32()[0]);
    }
    else if (is_a<ConstantF32>(src) && instr->get_Instrtype() == InstrutionEnum::F2I)
    {
        a = new ConstantI32(((ConstantF32 *)src)->get_f32()[0]);
    }
    else if (is_a<Assign>(instr))
    {
        a = src;
    }
    else
        return nullptr;

    assert(a);
    instr->get_BB()->get_func()->value_pushBack(a);
    instr->replaceAllUses(a);
    return a;
}
Value *const_fold_binary(Binary *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Constant *a = nullptr;
    if (is_a<ConstantI32>(src1) && is_a<ConstantI32>(src2))
    {
        int i1 = ((ConstantI32 *)src1)->get_i32()[0];
        int i2 = ((ConstantI32 *)src2)->get_i32()[0];
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD:
            a = new ConstantI32(i1 + i2); //+0
            break;
        case InstrutionEnum::ISUB:
            a = new ConstantI32(i1 - i2); //-0
            break;
        case InstrutionEnum::IMUL:
            a = new ConstantI32(i1 * i2); //*0
            break;
        case InstrutionEnum::IDIV:
            a = new ConstantI32(i1 / i2);
            break;
        case InstrutionEnum::IMOD:
            a = new ConstantI32(i1 % i2);
            break;
        default:
            assert(0);
        }
    }
    else if (is_a<ConstantF32>(src1) && is_a<ConstantF32>(src2))
    {
        float f1 = ((ConstantF32 *)src1)->get_f32()[0];
        float f2 = ((ConstantF32 *)src2)->get_f32()[0];
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::FADD:
            a = new ConstantF32(f1 + f2); //+0
            break;
        case InstrutionEnum::FSUB:
            a = new ConstantF32(f1 - f2); //-0
            break;
        case InstrutionEnum::FMUL:
            a = new ConstantF32(f1 * f2); //*0
            break;
        case InstrutionEnum::FDIV:
            a = new ConstantF32(f1 / f2);
            break;
        default:
            assert(0);
        }
    }
    else
        return nullptr;

    assert(a);
    instr->get_BB()->get_func()->value_pushBack(a);
    instr->replaceAllUses(a);
    return a;
}
Value *const_fold_Cmp(Cmp *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Constant *a = nullptr;
    if (is_a<ConstantI32>(src1) && is_a<ConstantI32>(src2))
    {
        int i1 = ((ConstantI32 *)src1)->get_i32()[0];
        int i2 = ((ConstantI32 *)src2)->get_i32()[0];
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IEQ:
            a = new ConstantF32(i1 == i2);
            break;
        case InstrutionEnum::INEQ:
            a = new ConstantF32(i1 != i2);
            break;
        case InstrutionEnum::IGT:
            a = new ConstantF32(i1 > i2);
            break;
        case InstrutionEnum::IGE:
            a = new ConstantF32(i1 >= i2);
            break;
        case InstrutionEnum::ILT:
            a = new ConstantF32(i1 < i2);
            break;
        case InstrutionEnum::ILE:
            a = new ConstantF32(i1 <= i2);
            break;
        default:
            assert(0);
        }
    }
    else if (is_a<ConstantF32>(src1) && is_a<ConstantF32>(src2))
    {
        int f1 = ((ConstantF32 *)src1)->get_f32()[0];
        int f2 = ((ConstantF32 *)src2)->get_f32()[0];
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::FEQ:
            a = new ConstantF32(f1 == f2);
            break;
        case InstrutionEnum::FNEQ:
            a = new ConstantF32(f1 != f2);
            break;
        case InstrutionEnum::FGT:
            a = new ConstantF32(f1 > f2);
            break;
        case InstrutionEnum::FGE:
            a = new ConstantF32(f1 >= f2);
            break;
        case InstrutionEnum::FLT:
            a = new ConstantF32(f1 < f2);
            break;
        case InstrutionEnum::FLE:
            a = new ConstantF32(f1 <= f2);
            break;
        default:
            assert(0);
        }
    }
    else
        return nullptr;

    assert(a);
    instr->get_BB()->get_func()->value_pushBack(a);
    instr->replaceAllUses(a);
    return a;
}