#include <ir/Instrution.hpp>
#include <ir/BasicBlock.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

std::unordered_map<InstrutionEnum, std::string> *Instrution::_symbol_map =
    new std::unordered_map<InstrutionEnum, std::string>{
        {InstrutionEnum::MINUS, "MINUS"},
        {InstrutionEnum::F2I, "F2I"},
        {InstrutionEnum::I2F, "I2F"},
        {InstrutionEnum::IADD, "+"},
        {InstrutionEnum::ISUB, "-"},
        {InstrutionEnum::IMUL, "*"},
        {InstrutionEnum::IDIV, "/"},
        {InstrutionEnum::IMOD, "%"},
        {InstrutionEnum::FADD, "+"},
        {InstrutionEnum::FSUB, "-"},
        {InstrutionEnum::FMUL, "*"},
        {InstrutionEnum::FDIV, "/"},
        {InstrutionEnum::SHL, "<<"},
        {InstrutionEnum::LSHR, ">>"},
        {InstrutionEnum::ASHR, "(Arithmetic)>>"},
        {InstrutionEnum::AND, "&"},
        {InstrutionEnum::OR, "|"},
        {InstrutionEnum::XOR, "^"},
        {InstrutionEnum::IEQ, "=="},
        {InstrutionEnum::INEQ, "!="},
        {InstrutionEnum::ILT, "<"},
        {InstrutionEnum::ILE, "<="},
        {InstrutionEnum::IGT, ">"},
        {InstrutionEnum::IGE, ">="},
        {InstrutionEnum::FEQ, "=="},
        {InstrutionEnum::FNEQ, "!="},
        {InstrutionEnum::FLT, "<"},
        {InstrutionEnum::FLE, "<="},
        {InstrutionEnum::FGT, ">"},
        {InstrutionEnum::FGE, ">="},
        {InstrutionEnum::AddSP, "SP +"}};

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type)
    : User(basic_type), parent(_BB), instr_type(type)
{
    _BB->Ins_pushBack(this);
    Function *p_func = _BB->get_func();
    assert(p_func != nullptr);
    p_func->value_pushBack((Value *)this);
}

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type, bool notPush)
    : User(basic_type), parent(_BB), instr_type(type)
{
    if (!notPush)
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

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, Type *p_array, bool _ele)
    : User(p_array, _ele), parent(_BB), instr_type(type)
{
    _BB->Ins_pushBack(this);
    Function *p_func = _BB->get_func();
    assert(p_func != nullptr);
    p_func->value_pushBack((Value *)this);
}

Alloca::Alloca(BasicBlock *_parent, TypeEnum type)
    : Instrution(_parent, InstrutionEnum::Alloca, type)
{
}

Alloca::Alloca(BasicBlock *_perant, p_symbol_var p_var)
    : Instrution(_perant, InstrutionEnum::Alloca, p_var)
{
}

GEP::GEP(Value *_addr, Value *_offset, bool _element, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::GEP, _addr->get_type(), _element), is_element(_element)
{
    Edge *p_in1 = new Edge(this, _addr);
    Edge *p_in2 = new Edge(this, _offset);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _addr->user_list_push_back(p_in1);
    _offset->user_list_push_back(p_in2);
}

Ret::Ret(BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Ret, TypeEnum::Void)
{
}

Ret::Ret(Value *_val, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Ret, _val->get_type()->get_type())
{
    Edge *p_in1 = new Edge(this, _val);
    value_list_push_back(p_in1);
    _val->user_list_push_back(p_in1);
}

BasicBlock *Jmp::get_nextBB()
{
    return (BasicBlock *)((*(this->get_parent()->get_user_list()))[0]->get_user());
}

Jmp::Jmp(BasicBlock *_next, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Jmp, TypeEnum::Void)
{
    Edge *p_in1 = new Edge(_next, _parent);
    _parent->user_list_push_back(p_in1);
    _next->value_list_push_back(p_in1);
}

BasicBlock *Branch::get_trueBB()
{
    return (BasicBlock *)((*(this->get_parent()->get_user_list()))[0]->get_user());
}

BasicBlock *Branch::get_falseBB()
{
    return (BasicBlock *)((*(this->get_parent()->get_user_list()))[1]->get_user());
}

Branch::Branch(Value *_cond, BasicBlock *_trueBB, BasicBlock *_falseBB, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Branch, TypeEnum::Void)
{
    Edge *p_in0 = new Edge(this, _cond);
    this->value_list_push_back(p_in0);
    _cond->user_list_push_back(p_in0);
    Edge *p_in1 = new Edge(_trueBB, _parent);
    Edge *p_in2 = new Edge(_falseBB, _parent);
    _parent->user_list_push_back(p_in1);
    _parent->user_list_push_back(p_in2);
    _trueBB->value_list_push_back(p_in1);
    _falseBB->value_list_push_back(p_in2);
}

Load::Load(Value *p_val, bool _is_stack_ptr, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Load, p_val->get_type(), true)
{
    assert(p_val->get_type()->get_type() == TypeEnum::Ptr);
    // assert(p_val->get_type()->get_basic_type() == TypeEnum::I32 || p_val->get_type()->get_basic_type() == TypeEnum::F32);
    is_stack_ptr = _is_stack_ptr;
    Edge *p_in = new Edge(this, p_val);
    value_list_push_back(p_in);
    p_val->user_list_push_back(p_in);
}

Store::Store(Value *_addr, Value *_src, bool _stack, BasicBlock *_BB)
    : Instrution(_BB, InstrutionEnum::Store, TypeEnum::Void), is_stack_ptr(_stack)
{
    Edge *p_in1 = new Edge(this, _addr);
    Edge *p_in2 = new Edge(this, _src);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _addr->user_list_push_back(p_in1);
    _src->user_list_push_back(p_in2);
}

Cmp::Cmp(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, TypeEnum::I32)
{
    Edge *p_in1 = new Edge(this, _src1);
    Edge *p_in2 = new Edge(this, _src2);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _src1->user_list_push_back(p_in1);
    _src2->user_list_push_back(p_in2);
}

Binary::Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, (type <= InstrutionEnum::IMOD ? TypeEnum::I32 : TypeEnum::F32))
{
    Edge *p_in1 = new Edge(this, _src1);
    Edge *p_in2 = new Edge(this, _src2);
    value_list_push_back(p_in1);
    value_list_push_back(p_in2);
    _src1->user_list_push_back(p_in1);
    _src2->user_list_push_back(p_in2);
}
Unary::Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent)
    : Instrution(_parent, type, (type == InstrutionEnum::AddSP ? TypeEnum::I32 : _src1->get_type()->get_type()))
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
        assert(src_type == TypeEnum::I32);
        this->get_type()->reset(TypeEnum::F32);
        break;
    case InstrutionEnum::AddSP:
        break;
    default:
        this->get_type()->reset(src_type);
        break;
    }
    Edge *p_in1 = new Edge(this, _src1);
    value_list_push_back(p_in1);
    _src1->user_list_push_back(p_in1);
}

Unary::Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent, bool notPush)
    : Instrution(_parent, type, (type == InstrutionEnum::AddSP ? TypeEnum::I32 : _src1->get_type()->get_type()), notPush)
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
        assert(src_type == TypeEnum::I32);
        this->get_type()->reset(TypeEnum::F32);
        break;
    case InstrutionEnum::AddSP:
        break;
    default:
        this->get_type()->reset(src_type);
        break;
    }
    Edge *p_in1 = new Edge(this, _src1);
    value_list_push_back(p_in1);
    _src1->user_list_push_back(p_in1);
}

Call::Call(Value *_func, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Call, _func->get_type()->get_type())
{
    Edge *p_in = new Edge(this, _func);
    value_list_push_back(p_in);
    _func->user_list_push_back(p_in);
}

void Call::params_pushback(Value *_param)
{
    params.emplace_back(_param);
}

// drop

Instrution::~Instrution()
{
}

// print

void Call::print()
{
    printf("    ");
    Value *p_func = (*this->get_value_list())[0]->get_val();
    if (p_func->get_type()->get_type() != TypeEnum::Void)
    {

        this->get_type()->print();
        printf(" %%%d = ", this->get_ID());
    }
    printf("call @");
    std::cout << ((GlobalValue *)p_func)->get_name();
    putchar('(');
    int n = params.size();
    for (int i = 0; i < n - 1; ++i)
    {
        params[i]->get_type()->print();
        putchar(' ');
        params[i]->print_ID();
        printf(", ");
    }
    if (n)
    {
        params[n - 1]->get_type()->print();
        putchar(' ');
        params[n - 1]->print_ID();
    }

    printf(")\n");
}

void GEP::print()
{
    printf("    ");
    this->get_type()->print();
    Value *p_addr = (*this->get_value_list())[0]->get_val();
    Value *p_offset = (*this->get_value_list())[1]->get_val();
    printf(" %%%d = getelementptr ", this->get_ID());
    p_addr->get_type()->print();
    putchar(' ');
    if (is_a<GlobalValue>(p_addr))
        std::cout << ((GlobalValue *)p_addr)->get_name();
    else
        printf("%%%d", p_addr->get_ID());
    if (is_element)
        printf(" i32 0 ");
    putchar(' ');
    p_offset->get_type()->print();
    putchar(' ');
    p_offset->print_ID();
    putchar('\n');
}

void Ret::print()
{
    printf("    ret ");
    std::vector<Edge *> *p_in = this->get_value_list();
    if (!p_in->empty())
        (*p_in)[0]->get_val()->print_ID();
    putchar('\n');
}

void Jmp::print()
{
    printf("    br lable b%d\n", (*(this->get_parent()->get_user_list()))[0]->get_user()->get_ID());
}

void Branch::print()
{
    printf("    br ");
    std::vector<Edge *> *_list = this->get_value_list();
    (*_list)[0]->get_val()->print_ID();
    _list = this->get_parent()->get_user_list();
    assert(_list->size() == 2);
    Value *trueBB = (*_list)[0]->get_user(), *falseBB = (*_list)[1]->get_user();
    printf(", lable b%d, lable b%d\n", trueBB->get_ID(), falseBB->get_ID());
}

void Load::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = load ", this->get_ID());
    fflush(stdout);
    Value *p_addr = get_addr();
    p_addr->get_type()->print();
    putchar(' ');
    if (is_a<GlobalValue>(p_addr))
        std::cout << ((GlobalValue *)p_addr)->get_name() << std::endl;
    else
        printf("%%%d\n", p_addr->get_ID());
}

void Store::print()
{
    printf("    ");
    printf("store ");
    Value *p_addr = get_addr();
    Value *p_src = get_src();
    p_addr->get_type()->print();
    putchar(' ');
    if (is_a<GlobalValue>(p_addr))
        std::cout << ((GlobalValue *)p_addr)->get_name();
    else
        printf("%%%d", p_addr->get_ID());
    putchar(' ');
    p_src->get_type()->print();
    putchar(' ');
    p_src->print_ID();
    putchar('\n');
}

void Alloca::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = alloca ", this->get_ID());
    Type *_mtype = this->get_type();
    _mtype->print();
    printf(", align ");
    if (_mtype->get_type() == TypeEnum::Ptr)
        printf("%d\n", 8);
    else
        printf("%d\n", 4);
}

void Cmp::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = ", this->get_ID());
    Value *p_src1 = get_src1(), *p_src2 = get_src2();
    p_src1->get_type()->print();
    putchar(' ');
    p_src1->print_ID();
    putchar(' ');
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    putchar(' ');
    p_src2->get_type()->print();
    putchar(' ');
    p_src2->print_ID();
    putchar('\n');
}

void Binary::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = ", this->get_ID());
    Value *p_src1 = get_src1(), *p_src2 = get_src2();
    p_src1->get_type()->print();
    putchar(' ');
    p_src1->print_ID();
    putchar(' ');
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    putchar(' ');
    p_src1->get_type()->print();
    putchar(' ');
    p_src2->print_ID();
    putchar('\n');
}

void Unary::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = ", this->get_ID());
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    putchar(' ');
    Value *p_src = get_src();
    p_src->get_type()->print();
    putchar(' ');
    p_src->print_ID();
    putchar('\n');
}

void Instrution::replaceAllUses(Value *RepVal)
{
    for (Edge *edge : *(get_user_list()))
    {
        edge->set_val(RepVal);
        RepVal->user_list_push_back(edge);
    }
    this->get_user_list()->clear();
}

void Instrution::drop()
{
    parent->erase_instr(this);
    Value::drop();
}

void PHINode::addIncoming(Value *val, BasicBlock *BB)
{
    Edge *edge = new Edge(this, val);
    this->value_list_push_back(edge);
    val->user_list_push_back(edge);
    valueMap->insert({BB, val});
}

PHINode::PHINode(BasicBlock *_BB, TypeEnum basic_type, bool notPush)
    : Instrution(_BB, InstrutionEnum::PHINode, basic_type, notPush)
{
    valueMap = new std::unordered_map<BasicBlock *, Value *>();
}

void PHINode::print()
{
    printf("    ");
    this->get_type()->print();
    printf(" %%%d = phi(", this->get_ID());
    int num = valueMap->size();
    for (auto it : *valueMap)
    {
        printf("{label : b%d, %%%d}", it.first->get_ID(), it.second->get_ID());
        num--;
        if (num)
            printf(", ");
    }
    printf(")\n");
}

PHINode::~PHINode()
{
    delete valueMap;
}

void Assign::print()
{
    Unary::print();
}
