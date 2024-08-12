#include <iostream>
#include <vector>

#include "../../include/ir/Instrution.hpp"
#include "../../include/ir/BasicBlock.hpp"
#include "../../include/ir/Constant.hpp"
#include "ir/Edge.hpp"

std::unordered_map<InstrutionEnum, std::string> *Instrution::_symbol_map =
    new std::unordered_map<InstrutionEnum, std::string>{
        {InstrutionEnum::MINUS, "neg"},
        {InstrutionEnum::F2I, "fptosi"},
        {InstrutionEnum::I2F, "sitofp"},
        {InstrutionEnum::IADD, "add nsw"},
        {InstrutionEnum::ISUB, "sub nsw"},
        {InstrutionEnum::IMUL, "mul nsw"},
        {InstrutionEnum::IDIV, "sdiv"},
        {InstrutionEnum::IMOD, "srem"},
        {InstrutionEnum::FADD, "fadd"},
        {InstrutionEnum::FSUB, "fsub"},
        {InstrutionEnum::FMUL, "fmul"},
        {InstrutionEnum::FDIV, "fdiv"},
        {InstrutionEnum::PADD, "add ptr"},
        {InstrutionEnum::SHL, "<<"},
        {InstrutionEnum::LSHR, ">>"},
        {InstrutionEnum::ASHR, "(Arithmetic)>>"},
        {InstrutionEnum::AND, "&"},
        {InstrutionEnum::OR, "|"},
        {InstrutionEnum::XOR, "^"},
        {InstrutionEnum::IEQ, "icmp eq"},
        {InstrutionEnum::INEQ, "icmp ne"},
        {InstrutionEnum::ILT, "icmp slt"},
        {InstrutionEnum::ILE, "icmp sle"},
        {InstrutionEnum::IGT, "icmp sgt"},
        {InstrutionEnum::IGE, "icmp sge"},
        {InstrutionEnum::FEQ, "fcmp oeq"},
        {InstrutionEnum::FNEQ, "fcmp une"},
        {InstrutionEnum::FLT, "fcmp olt"},
        {InstrutionEnum::FLE, "fcmp ole"},
        {InstrutionEnum::FGT, "fcmp ogt"},
        {InstrutionEnum::FGE, "fcmp oge"},
        {InstrutionEnum::AddSP, "SP +"}};

void Instrution::replaceInstr(BasicBlock *_BB, int pos) // 适用新建在末尾的指令使用或者非相同块的指令替换，否则迭代器失效
{
    std::vector<Instrution *> *_instrs = this->get_parent()->get_instrutions();
    if (_instrs->back() == this)
        _instrs->pop_back();
    else
    {
        for (auto it = _instrs->begin(); it != _instrs->end(); it++)
            if (*it == this)
            {
                _instrs->erase(it);
                break;
            }
    }
    this->parent = _BB;
    Value *tmp = (*_BB->get_instrutions())[pos];
    for (Edge *_edge : (*tmp->get_user_list()))
    {
        _edge->set_val(this);
        // this->user_list_push_back(_edge);
    }
    tmp->get_user_list()->clear();
    (*_BB->get_instrutions())[pos] = this;
}

void Instrution::insertInstr(BasicBlock *_BB, int pos)
{
    std::vector<Instrution *> *_instrs = this->get_parent()->get_instrutions();

    int p = 0;
    for (auto it = _instrs->begin(); it != _instrs->end(); it++, p++)
    {
        if (*it == this)
        {
            _instrs->erase(it);
            break;
        }
    }
    if (pos >= p && _BB == parent)
        pos--;

    this->parent = _BB;
    _instrs = _BB->get_instrutions();
    _instrs->insert(_instrs->begin() + pos, this);
}

void Instrution::removeInstr()
{
    std::vector<Instrution *> *_instrs = this->get_parent()->get_instrutions();
    if (_instrs->back() == this)
        _instrs->pop_back();
    else
    {
        for (auto it = _instrs->begin(); it != _instrs->end(); it++)
            if (*it == this)
            {
                _instrs->erase(it);
                break;
            }
    }
    this->parent = nullptr;
}

bool Instrution::is_commutative()
{
    if (this->get_Instrtype() == InstrutionEnum::IADD ||
        this->get_Instrtype() == InstrutionEnum::FADD)
        return true;
    if (this->get_Instrtype() == InstrutionEnum::IMUL ||
        this->get_Instrtype() == InstrutionEnum::FMUL)
        return true;
    // TODO:bit
    return false;
}
Value *Instrution::get_operand_at(int pos)
{
    assert(this->get_value_list()->size() > pos);
    return get_value_list()->at(pos)->get_val();
}

void Instrution::move_before_terminator()
{
    int pos = get_parent()->get_instrs()->size() - 1;
    if (get_parent()->get_last_instrution()->isBranch())
        pos--;
    insertInstr(get_parent(), pos);
}
// include instr and phinode, plz read before use
int Instrution::get_pos_of_bb()
{
    if (is_a<PHINode>(this))
    {
        auto vec = get_parent()->get_phinodes();
        for (int i = 0; i < vec->size(); i++)
        {
            if (vec->at(i) == this)
                return i;
        }
    }
    auto vec = get_parent()->get_instrutions();
    for (int i = 0; i < vec->size(); i++)
    {
        if (vec->at(i) == this)
            return i;
    }

    assert(0);
}

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

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, Type *_type, int is_copy, bool notPush)
    : User(_type, is_copy), parent(_BB), instr_type(type)
{
    if (!notPush)
        _BB->Ins_pushBack(this);
    Function *p_func = _BB->get_func();
    assert(p_func != nullptr);
    p_func->value_pushBack((Value *)this);
}

Instrution::Instrution(BasicBlock *_BB, InstrutionEnum type, Type *_type, int is_copy)
    : User(_type, is_copy), parent(_BB), instr_type(type)
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

Alloca::Alloca(BasicBlock *_parent, Type *_type, int is_copy)
    : Instrution(_parent, InstrutionEnum::Alloca, _type, is_copy)
{
    assert(is_copy == 1);
}

Alloca::Alloca(BasicBlock *_parent, Type *_type)
    : Instrution(_parent, InstrutionEnum::Alloca, _type, false)
{
}

GEP::GEP(Value *_addr, Value *_offset, bool _element, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::GEP, _addr->get_type(), _element), is_element(_element)
{
    // Edge *p_in1 = new Edge(this, _addr);
    // Edge *p_in2 = new Edge(this, _offset);
    // value_list_push_back(p_in1);
    // value_list_push_back(p_in2);
    // _addr->user_list_push_back(p_in1);
    // _offset->user_list_push_back(p_in2);
    new Edge(this, _addr);
    new Edge(this, _offset);
}

Ret::Ret(BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Ret, TypeEnum::Void)
{
}

Ret::Ret(Value *_val, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Ret, _val->get_type()->get_type())
{
    // Edge *p_in1 = new Edge(this, _val);
    // value_list_push_back(p_in1);
    // _val->user_list_push_back(p_in1);
    new Edge(this, _val);
}

BasicBlock *Jmp::get_nextBB()
{
    return (BasicBlock *)((*(this->get_parent()->get_user_list()))[0]->get_user());
}

Jmp::Jmp(BasicBlock *_next, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Jmp, TypeEnum::Void)
{
    // Edge *p_in1 = new Edge(_next, _parent);
    // _parent->user_list_push_back(p_in1);
    // _next->value_list_push_back(p_in1);
    new Edge(_next, _parent);
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
    // Edge *p_in0 = new Edge(this, _cond);
    // this->value_list_push_back(p_in0);
    // _cond->user_list_push_back(p_in0);
    // Edge *p_in1 = new Edge(_trueBB, _parent);
    // Edge *p_in2 = new Edge(_falseBB, _parent);
    // _parent->user_list_push_back(p_in1);
    // _parent->user_list_push_back(p_in2);
    // _trueBB->value_list_push_back(p_in1);
    // _falseBB->value_list_push_back(p_in2);
    new Edge(this, _cond);
    new Edge(_trueBB, _parent);
    new Edge(_falseBB, _parent);
}

Load::Load(Value *p_val, bool _is_stack_ptr, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Load, p_val->get_type(), true)
{
    assert(p_val->get_type()->get_type() == TypeEnum::Ptr);
    // assert(p_val->get_type()->get_basic_type() == TypeEnum::I32 || p_val->get_type()->get_basic_type() == TypeEnum::F32);
    is_stack_ptr = _is_stack_ptr;
    // Edge *p_in = new Edge(this, p_val);
    // value_list_push_back(p_in);
    // p_val->user_list_push_back(p_in);
    new Edge(this, p_val);
}

Store::Store(Value *_addr, Value *_src, bool _stack, BasicBlock *_BB)
    : Instrution(_BB, InstrutionEnum::Store, TypeEnum::Void), is_stack_ptr(_stack)
{
    // Edge *p_in1 = new Edge(this, _addr);
    // Edge *p_in2 = new Edge(this, _src);
    // value_list_push_back(p_in1);
    // value_list_push_back(p_in2);
    // _addr->user_list_push_back(p_in1);
    // _src->user_list_push_back(p_in2);
    new Edge(this, _addr);
    new Edge(this, _src);
}

Cmp::Cmp(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, TypeEnum::I1)
{
    // Edge *p_in1 = new Edge(this, _src1);
    // Edge *p_in2 = new Edge(this, _src2);
    // value_list_push_back(p_in1);
    // value_list_push_back(p_in2);
    // _src1->user_list_push_back(p_in1);
    // _src2->user_list_push_back(p_in2);
    new Edge(this, _src1);
    new Edge(this, _src2);
}

bool Cmp::isCond()
{
    if (this->get_user_list()->size() == 1)
    {
        assert(is_a<Instrution>(this->get_user_list()->at(0)->get_user()));
        if (((Instrution *)this->get_user_list()->at(0)->get_user())->isBranch())
            return true;
    }
    return false;
}
Binary::Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type,
                 (_src1->get_type()->get_type() == TypeEnum::I1 || _src2->get_type()->get_type() == TypeEnum::I1)
                     ? TypeEnum::I1
                     : ((type <= InstrutionEnum::IMOD || type > InstrutionEnum::FDIV) ? TypeEnum::I32 : TypeEnum::F32))
{
    // Edge *p_in1 = new Edge(this, _src1);
    // Edge *p_in2 = new Edge(this, _src2);
    // value_list_push_back(p_in1);
    // value_list_push_back(p_in2);
    // _src1->user_list_push_back(p_in1);
    // _src2->user_list_push_back(p_in2);
    new Edge(this, _src1);
    new Edge(this, _src2);
}

Binary::Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent, bool is_ele)
    : Instrution(_parent, InstrutionEnum::PADD, _src1->get_type(), is_ele)
{
    // Edge *p_in1 = new Edge(this, _src1);
    // Edge *p_in2 = new Edge(this, _src2);
    // value_list_push_back(p_in1);
    // value_list_push_back(p_in2);
    // _src1->user_list_push_back(p_in1);
    // _src2->user_list_push_back(p_in2);
    new Edge(this, _src1);
    new Edge(this, _src2);
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
        assert(src_type == TypeEnum::I32 || src_type == TypeEnum::I1);
        this->get_type()->reset(TypeEnum::F32);
        break;
    case InstrutionEnum::AddSP:
        break;
    default:
        this->get_type()->reset(src_type);
        break;
    }
    // Edge *p_in1 = new Edge(this, _src1);
    // value_list_push_back(p_in1);
    // _src1->user_list_push_back(p_in1);
    new Edge(this, _src1);
}

Unary::Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent, bool notPush)
    : Instrution(_parent, type, _src1->get_type(), 0, notPush)
{
    assert(type != InstrutionEnum::AddSP);
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
        assert(src_type == TypeEnum::I32 || src_type == TypeEnum::I1);
        this->get_type()->reset(TypeEnum::F32);
        break;
    case InstrutionEnum::AddSP:
        break;
    default:
        this->get_type()->reset(src_type);
        break;
    }
    // Edge *p_in1 = new Edge(this, _src1);
    // value_list_push_back(p_in1);
    // _src1->user_list_push_back(p_in1);
    new Edge(this, _src1);
}

Call::Call(Value *_func, BasicBlock *_parent)
    : Instrution(_parent, InstrutionEnum::Call, _func->get_type()->get_type())
{
    // Edge *p_in = new Edge(this, _func);
    // value_list_push_back(p_in);
    // _func->user_list_push_back(p_in);
    new Edge(this, _func);
}

void Call::params_pushback(Value *_param)
{
    // Edge *p_in1 = new Edge(this, _param);
    // this->value_list_push_back(p_in1);
    // _param->user_list_push_back(p_in1);
    new Edge(this, _param);
}

// drop

Instrution::~Instrution()
{
}

// print

static inline void print_align(TypeEnum type)
{
    switch (type)
    {
    case TypeEnum::I32:
    case TypeEnum::F32:
        printf(", align 4\n");
        break;
    case TypeEnum::Ptr:
        printf(", align 8\n");
        break;
    case TypeEnum::Array:
        printf(", align 16\n");
        break;
    default:
        assert(0);
        break;
    }
}

void Call::print()
{
    printf("    ");
    Value *p_func = (*this->get_value_list())[0]->get_val();
    if (p_func->get_type()->get_type() != TypeEnum::Void)
    {

        printf("%%%d = ", this->get_ID());
    }
    printf("call ");
    p_func->get_type()->print();
    std::cout << " @" << ((GlobalValue *)p_func)->get_name();
    putchar('(');
    std::vector<Edge *> *params = this->get_value_list();
    int n = params->size();
    for (int i = 1; i < n - 1; ++i)
    {
        (*params)[i]->get_val()->get_type()->print();
        printf(" noundef ");
        (*params)[i]->get_val()->print_ID();
        printf(", ");
    }
    if (n > 1)
    {
        (*params)[n - 1]->get_val()->get_type()->print();
        printf(" noundef ");
        (*params)[n - 1]->get_val()->print_ID();
    }

    printf(")\n");
}

void GEP::print()
{
    Value *p_addr = (*this->get_value_list())[0]->get_val();
    printf("    %%%d = getelementptr inbounds ", this->get_ID());
    if (p_addr->get_type()->get_type() != TypeEnum::Ptr)
    {
        puts("");
        p_addr->print();
        puts("");
        fflush(stdout);
        assert(0);
    }
    assert(p_addr->get_type()->get_type() == TypeEnum::Ptr);
    ((Ptr *)p_addr->get_type())->print_btype();
    printf(", ptr ");
    if (is_a<GlobalValue>(p_addr))
        std::cout << '@' << ((GlobalValue *)p_addr)->get_name();
    else
        printf("%%%d", p_addr->get_ID());
    if (is_element)
        printf(", i32 0 ");
    for (int i = 1; i < get_value_list()->size(); i++)
    {

        Value *p_offset = (*this->get_value_list())[i]->get_val();
        printf(", i32 ");
        p_offset->print_ID();
    }
    putchar('\n');
}

void Ret::print()
{
    assert(this->get_parent() == this->get_parent()->get_func()->get_retBB());
    printf("    ret ");
    std::vector<Edge *> *p_in = this->get_value_list();
    this->get_type()->print();
    putchar(' ');
    if (!p_in->empty())
        (*p_in)[0]->get_val()->print_ID();
    putchar('\n');
}

void Jmp::print()
{
    printf("    br label %%b%d\n", (*(this->get_parent()->get_user_list()))[0]->get_user()->get_ID());
}

void Branch::print()
{
    printf("    br i1 ");
    std::vector<Edge *> *_list = this->get_value_list();
    (*_list)[0]->get_val()->print_ID();
    _list = this->get_parent()->get_user_list();
    assert(_list->size() == 2);
    Value *trueBB = (*_list)[0]->get_user(), *falseBB = (*_list)[1]->get_user();
    printf(", label %%b%d, label %%b%d\n", trueBB->get_ID(), falseBB->get_ID());
}

void Load::print()
{
    printf("    %%%d = load ", this->get_ID());
    Value *p_addr = get_addr();
    ((Ptr *)p_addr->get_type())->print_btype();
    printf(", ptr ");
    if (is_a<GlobalValue>(p_addr))
        std::cout << '@' << ((GlobalValue *)p_addr)->get_name();
    else
        printf("%%%d", p_addr->get_ID());
    print_align(this->get_type()->get_type());
}

void Store::print()
{
    printf("    store ");
    Value *p_addr = get_addr();
    Value *p_src = get_src();
    p_src->get_type()->print();
    putchar(' ');
    p_src->print_ID();
    printf(", ptr ");
    if (is_a<GlobalValue>(p_addr))
        std::cout << "@" << ((GlobalValue *)p_addr)->get_name();
    else
        printf("%%%d", p_addr->get_ID());
    print_align(p_src->get_type()->get_type());
}

void Alloca::print()
{
    printf("    %%%d = alloca ", this->get_ID());
    Type *_mtype = this->get_type();
    assert(_mtype->get_type() == TypeEnum::Ptr);
    _mtype = ((Ptr *)_mtype)->get_btype();
    _mtype->print();
    print_align(_mtype->get_type());

    if (_mtype->get_type() == TypeEnum::Array)
    {
        std::cout << "    call void @llvm.memset.p0.i64(ptr align 16 ";
        this->print_ID();
        std::cout << ", i8 0, i64 ";
        printf("%d", ((ArrayType *)_mtype)->get_size() << 2);
        std::cout << ", i1 false)" << std::endl;
    }
}

void Cmp::print()
{
    printf("    %%%d = ", this->get_ID());
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    putchar(' ');
    Value *p_src1 = get_src1(), *p_src2 = get_src2();
    if (is_a<Cmp>(p_src1) || is_a<Cmp>(p_src2))
        printf("i1");
    else
        p_src1->get_type()->print();
    putchar(' ');
    p_src1->print_ID();
    printf(", ");
    p_src2->print_ID();
    putchar('\n');
}

void Binary::print()
{
    printf("    %%%d = ", this->get_ID());
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    putchar(' ');
    Value *p_src1 = get_src1(), *p_src2 = get_src2();
    if (is_a<Cmp>(p_src1) || is_a<Cmp>(p_src2))
        printf("i1");
    else
        this->get_type()->print();
    putchar(' ');
    if (is_a<GlobalValue>(p_src1))
        std::cout << "@" << ((GlobalValue *)p_src1)->get_name();
    else
        p_src1->print_ID();
    putchar(',');
    putchar(' ');
    p_src2->print_ID();
    putchar('\n');
}

void Unary::print()
{
    printf("    %%%d = ", this->get_ID());
    if (get_Instrtype() == InstrutionEnum::MINUS)
    {
        if (this->get_type()->get_type() == TypeEnum::I32)
            printf("sub nsw i32 0, ");
        else if (this->get_type()->get_type() == TypeEnum::I1)
            printf("sub nsw i1 0, ");
        else
            printf("fsub float 0.000000e+00, ");

        get_src()->print_ID();
        putchar('\n');
        return;
    }
    std::cout << (*_symbol_map)[this->get_Instrtype()];
    assert(this->get_Instrtype() != InstrutionEnum::Assign);
    putchar(' ');
    Value *p_src = get_src();
    p_src->get_type()->print();
    putchar(' ');
    p_src->print_ID();
    switch (this->get_Instype())
    {
    case InstrutionEnum::F2I:
        printf(" to i32");
        break;
    case InstrutionEnum::I2F:
        printf(" to float");
        break;
    default:

        break;
    }
    putchar('\n');
}

void Instrution::replaceAllUses(Value *RepVal)
{
    for (Edge *edge : *(get_user_list()))
    {
        edge->set_val(RepVal);
        // RepVal->user_list_push_back(edge);
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
    // Edge *edge = new Edge(this, val);
    // this->value_list_push_back(edge);
    // val->user_list_push_back(edge);
    Edge *t = new Edge(this, val);
    valueMap->insert({BB, t});
}
void PHINode::eraseIncoming(BasicBlock *BB)
{
    for (auto &kv : *valueMap)
        if (kv.first == BB)
        {
            kv.second->drop();
            break;
        }
}
PHINode::PHINode(BasicBlock *_BB, TypeEnum basic_type, bool notPush)
    : Instrution(_BB, InstrutionEnum::PHINode, basic_type, notPush)
{
    valueMap = new std::unordered_map<BasicBlock *, Edge *>();
}

PHINode::PHINode(BasicBlock *_BB, Type *_type, bool notPush)
    : Instrution(_BB, InstrutionEnum::PHINode, _type, 0, notPush)
{
    valueMap = new std::unordered_map<BasicBlock *, Edge *>();
}

BasicBlock *PHINode::get_edge_income_block(Edge *e)
{
    for (auto kv : *valueMap)
        if (kv.second == e)
            return kv.first;
    assert(0);
}

void PHINode::print()
{
    printf("    %%%d = phi ", this->get_ID());
    this->get_type()->print();
    int num = valueMap->size();
    for (auto it : *valueMap)
    {
        printf(" [ ");
        it.second->get_val()->print_ID();
        printf(", %%b%d]", it.first->get_ID());
        num--;
        if (num)
            printf(",");
    }
    printf("\n");
}

PHINode::~PHINode()
{
    delete valueMap;
}

void PHINode::drop()
{
    get_BB()->erase_phi(this);
    Value::drop();
}

void Assign::print()
{
    printf("    %%%d = ", this->get_ID());
    Value *p_src = get_src();
    Type *p_type = get_type();
    assert(p_type->get_type() == TypeEnum::I32 || p_type->get_type() == TypeEnum ::F32 || p_type->get_type() == TypeEnum::Ptr);
    if (p_type->get_type() == TypeEnum::I32)
    {
        printf("add nsw i32 ");
        p_src->print_ID();
        printf(", 0\n");
    }
    else if (p_type->get_type() == TypeEnum::F32)
    {
        printf("fadd float  ");
        p_src->print_ID();
        printf(", 0.000000e+00\n");
    }
    else if (p_type->get_type() == TypeEnum::Ptr)
    {
        printf("ptr todo");
        if (is_a<GlobalValue>(p_src))
            std::cout << '@' << ((GlobalValue *)p_src)->get_name();
        else
            p_src->print_ID();
        printf("\n");
    }
}

Move::Move(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent)
    : Instrution(_parent, type, TypeEnum::Void)
{
    new Edge(this, _src1);
    new Edge(this, _src2);
}

void Move::print()
{
    printf("mov");
    putchar(' ');
    Value *p_src1 = get_src1(), *p_src2 = get_src2();
    if (is_a<Cmp>(p_src1) || is_a<Cmp>(p_src2))
        printf("i1");
    else
        this->get_type()->print();
    putchar(' ');
    p_src1->print_ID();
    putchar(',');
    putchar(' ');
    p_src2->print_ID();
    putchar('\n');
}

Triple::Triple(InstrutionEnum type, Value *src1, Value *src2, Value *src3, BasicBlock *parent)
    : Instrution(parent, type, src1->get_type(), (int)0)
{
    new Edge(this, src1);
    new Edge(this, src2);
    new Edge(this, src3);
}

void Triple::print()
{
    printf("    %%%d = ", this->get_ID());
    get_operand_at(0)->print_ID();
    printf(" + ");
    get_operand_at(1)->print_ID();
    if (get_Instrtype() == InstrutionEnum::MLA)
        printf(" * ");
    else if (get_Instrtype() == InstrutionEnum::ADDlsl)
        printf(" << ");
    else
        assert(0);
    get_operand_at(2)->print_ID();
    printf("\n");
}
