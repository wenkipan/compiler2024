#include <ir/Instrution.hpp>
#include <ir/BasicBlock.hpp>

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type)
    : User(basic_type), parent(_BB), instr_type(type)
{
    _BB->Ins_pushBack(this);
    Function *p_func = _BB->get_func();
    assert(p_func != nullptr);
    p_func->value_pushBack((Value *)this);
}

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, p_symbol_var p_var)
    : User(p_var), parent(_BB), instr_type(type)
{
    _BB->Ins_pushBack(this);
    Function *p_func = _BB->get_func();
    assert(p_func != nullptr);
    p_func->value_pushBack((Value *)this);
}

Alloc::Alloc(BasicBlock *_parent, TypeEnum type)
    : Instrution(_parent, InstrutionEnum::Alloc, type)
{
}

Alloc::Alloc(BasicBlock *_perant, p_symbol_var p_var)
    : Instrution(_perant, InstrutionEnum::Alloc, p_var)
{
}

GEP::GEP(Value *_addr, Value *_offset, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::GEP, _addr->get_type()->get_type()), p_addr(_addr), p_offset(_offset)
{
    Edge *p_in1 = new Edge(this, _addr);
    Edge *p_in2 = new Edge(this, _offset);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _addr->user_list_push_back(p_in1);
    _offset->user_list_push_back(p_in2);
}

Jmp::Jmp(BasicBlock *_parent, BasicBlock *_next)
    : Instrution(_parent, InstrutionEnum::Jmp, TypeEnum::Void),
      nextBB(_next)
{
    _next->prevBB_add(_parent);
}

Branch::Branch(Value *_cond, BasicBlock *_trueBB, BasicBlock *_falseBB, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Branch, TypeEnum::Void),
      cond(_cond), trueBB(_trueBB), falseBB(_falseBB)
{
    _trueBB->prevBB_add(_parent);
    _falseBB->prevBB_add(_parent);
}

Load::Load(Value *p_val, bool _is_stack_ptr, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Load, ((p_val->get_type()->get_type() == TypeEnum::PtrF32) ? TypeEnum::F32 : TypeEnum::I32))
{
    is_stack_ptr = _is_stack_ptr;
    p_addr = p_val;
    Edge *p_in = new Edge(this, p_val);
    value_list_push_back(p_in);
    p_val->user_list_push_back(p_in);
}

Store::Store(Value *_addr, Value *_src, bool _stack, BasicBlock *_BB)
    : Instrution(_BB, InstrutionEnum::Store, TypeEnum::Void), p_addr(_addr), p_src(_src), is_stack_ptr(_stack)
{
    Edge *p_in1 = new Edge(this, _addr);
    Edge *p_in2 = new Edge(this, _src);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _addr->user_list_push_back(p_in1);
    _src->user_list_push_back(p_in2);
}

Cmp::Cmp(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, TypeEnum::I32), p_src1(_src1), p_src2(_src2)
{
    Edge *p_in1 = new Edge(this, _src1);
    Edge *p_in2 = new Edge(this, _src2);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _src1->user_list_push_back(p_in1);
    _src2->user_list_push_back(p_in2);
}

Binary::Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, (type <= InstrutionEnum::IMOD ? TypeEnum::I32 : TypeEnum::F32)),
      p_src1(_src1),
      p_src2(_src2)
{
    Edge *p_in1 = new Edge(this, _src1);
    Edge *p_in2 = new Edge(this, _src2);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _src1->user_list_push_back(p_in1);
    _src2->user_list_push_back(p_in2);
}
Unary::Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent)
    : Instrution(_parent, type, _src1->get_type()->get_type()),
      p_src(_src1)
{
    TypeEnum src_type = _src1->get_type()->get_type();
    switch (type)
    {
    case InstrutionEnum::MINUS:
        break;
    case InstrutionEnum::F2I:
        assert(src_type == TypeEnum::F32);
        this->get_type()->reset(TypeEnum::I32);
        break;
    case InstrutionEnum::I2F:
        this->get_type()->reset(TypeEnum::F32);
        break;
    default:
        assert(0);
        break;
    }
    Edge *p_in1 = new Edge(this, _src1);
    value_list_push_back(p_in1);
    _src1->user_list_push_back(p_in1);
}

Call::Call(Value *_func, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Call, _func->get_type()->get_type()),
      p_func(_func)
{
    Edge *p_in = new Edge(this, _func);
    value_list_push_back(p_in);
    _func->user_list_push_back(p_in);
}

void Call::params_pushback(Value *_param)
{
    params.emplace_back(_param);
}