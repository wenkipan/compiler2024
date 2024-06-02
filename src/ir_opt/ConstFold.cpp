#include "ir/Constant.hpp"
#include "ir/Value.hpp"
#include <ir_opt/ConstFold.hpp>
#include <type_traits>

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
    { // GEP can be folded but i keep it Decompose utill opt like GVN was done
        return nullptr;
    }
    else if (instr->isCall())
    { // TODO: call with all-const can be folded //update: inline can do this
        return nullptr;
    }
    else if (instr->isLoad() || instr->isStore())
    { // TODO: wait memryssa
        return nullptr;
    }
    else if (instr->isBranch())
    { // already done in SCCP,maybe can do this here
        return nullptr;
    }
    else if (instr->isAlloca() || instr->isReturn() || instr->isJmp())
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
template <typename ConstX32, typename basic>
static Value *get_replace_value_when_src1_const(Binary *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Value *a = nullptr;
    basic i = ((ConstX32 *)src1)->get_32_at(0);
    if (i == (basic)0)
    {
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD: //  0+a
        case InstrutionEnum::FADD:
            a = src2;
            break;
        case InstrutionEnum::IMUL:
        case InstrutionEnum::FMUL:
            a = new ConstX32((basic)0); // 0*a
            break;
        case InstrutionEnum::IDIV:
        case InstrutionEnum::FDIV:
            a = new ConstX32((basic)0); // 0/a
            break;
        default:
            break;
        }
    }
    else if (i == (basic)1)
    {
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IMUL: // 1*a
        case InstrutionEnum::FMUL:
            a = src2;
            break;
        default:
            break;
        }
    }
    return a;
}
template <typename ConstX32, typename basic>
static Value *get_replace_value_when_src2_const(Binary *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Value *a = nullptr;
    basic i = ((ConstX32 *)src2)->get_32_at(0);
    if (i == (basic)0)
    {
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IADD: // a+0
        case InstrutionEnum::FADD:
            a = src1;
            break;
        case InstrutionEnum::ISUB: // a-0
        case InstrutionEnum::FSUB:
            a = src1;
            break;
        case InstrutionEnum::IMUL: // a*0
        case InstrutionEnum::FMUL:
            a = new ConstX32((basic)0);
            break;
            break;
        default:
            break;
        }
    }
    else if (i == (basic)1)
    {
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::IMUL: //  a*1
        case InstrutionEnum::FMUL:
            a = src1;
            break;
        case InstrutionEnum::IDIV: // a/1
        case InstrutionEnum::FDIV:
            a = src1;
            break;
        default:
            break;
        }
    }
    return a;
}
template <typename ConstX32, typename basic>
static Value *get_replace_value_when_both_const(Binary *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Value *a = nullptr;
    basic i1 = ((ConstX32 *)src1)->get_32_at(0);
    basic i2 = ((ConstX32 *)src2)->get_32_at(0);
    switch (instr->get_Instrtype())
    {
    case InstrutionEnum::IADD:
    case InstrutionEnum::FADD:
        a = new ConstX32(i1 + i2); // a+0 0+a
        break;
    case InstrutionEnum::ISUB:
    case InstrutionEnum::FSUB:
        a = new ConstX32(i1 - i2); // a-0
        break;
    case InstrutionEnum::IMUL:
    case InstrutionEnum::FMUL:
        a = new ConstX32(i1 * i2); // a*0 0*a 1*a a*1
        break;
    case InstrutionEnum::IDIV:
    case InstrutionEnum::FDIV:
        a = new ConstX32(i1 / i2); // a/1 ,0/a
        break;
    case InstrutionEnum::IMOD:
        if (std::is_same<ConstX32, ConstantF32>::value)
            assert(1);
        a = new ConstX32((int)i1 % (int)i2); // warning it s risky
        break;
    default:
        assert(0);
    }
    assert(a);
    return a;
}
Value *const_fold_binary(Binary *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Value *a = nullptr;
    if (is_a<ConstantI32>(src1) && is_a<ConstantI32>(src2))
    {
        a = get_replace_value_when_both_const<ConstantI32, int>(instr);
    }
    else if (is_a<ConstantI32>(src1))
    {
        a = get_replace_value_when_src1_const<ConstantI32, int>(instr);
    }
    else if (is_a<ConstantI32>(src2))
    {
        a = get_replace_value_when_src2_const<ConstantI32, int>(instr);
    }
    else if (is_a<ConstantF32>(src1) && is_a<ConstantF32>(src2))
    {
        a = get_replace_value_when_both_const<ConstantF32, float>(instr);
    }
    else if (is_a<ConstantF32>(src1))
    {
        a = get_replace_value_when_src1_const<ConstantF32, float>(instr);
    }
    else if (is_a<ConstantF32>(src2))
    {
        a = get_replace_value_when_src2_const<ConstantF32, float>(instr);
    }
    else
        return nullptr;

    if (a)
    {
        if (is_a<Constant>(a))
            instr->get_BB()->get_func()->value_pushBack(a);
        instr->replaceAllUses(a);
    }
    return a;
}

template <typename ConstX32, typename basic>
static Constant *get_replace_value_cmp(Cmp *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Constant *a;
    basic i1 = ((ConstX32 *)src1)->get_32_at(0);
    basic i2 = ((ConstX32 *)src2)->get_32_at(0);
    switch (instr->get_Instrtype())
    {
    case InstrutionEnum::IEQ:
    case InstrutionEnum::FEQ:
        a = new ConstantI32(i1 == i2);
        break;
    case InstrutionEnum::INEQ:
    case InstrutionEnum::FNEQ:
        a = new ConstantI32(i1 != i2);
        break;
    case InstrutionEnum::IGT:
    case InstrutionEnum::FGT:
        a = new ConstantI32(i1 > i2);
        break;
    case InstrutionEnum::IGE:
    case InstrutionEnum::FGE:
        a = new ConstantI32(i1 >= i2);
        break;
    case InstrutionEnum::ILT:
    case InstrutionEnum::FLT:
        a = new ConstantI32(i1 < i2);
        break;
    case InstrutionEnum::ILE:
    case InstrutionEnum::FLE:
        a = new ConstantI32(i1 <= i2);
        break;
    default:
        assert(0);
    }
    assert(a);
    return a;
}

Value *const_fold_Cmp(Cmp *instr)
{
    Value *src1 = instr->get_src1();
    Value *src2 = instr->get_src2();
    Constant *a = nullptr;
    if (is_a<ConstantI32>(src1) && is_a<ConstantI32>(src2))
    {
        a = get_replace_value_cmp<ConstantI32, int>(instr);
    }
    else if (is_a<ConstantF32>(src1) && is_a<ConstantF32>(src2))
    {
        a = get_replace_value_cmp<ConstantF32, float>(instr);
    }
    else
        return nullptr;

    assert(a);
    assert(is_a<Constant>(a));
    instr->get_BB()->get_func()->value_pushBack(a);
    instr->replaceAllUses(a);
    return a;
}