#pragma once

#include <ir/User.hpp>
#include <unordered_map>

class BasicBlock;

// maybe change to Binarybegin  Binaryend
enum class InstrutionEnum
{
    Call,
    GEP,
    Jmp,
    Branch,
    PHINode,
    Ret,
    Load,
    Store,
    Alloca,
    CmpBegin,
    IEQ,
    INEQ,
    IGT,
    IGE,
    ILT,
    ILE,
    FEQ,
    FNEQ,
    FGT,
    FGE,
    FLT,
    FLE,
    CmpEnd,
    BinaryBegin,
    IADD,
    ISUB,
    IMUL,
    IDIV,
    IMOD,
    FADD,
    FSUB,
    FMUL,
    FDIV,
    // ptr add
    PADD,
    // bitwise
    SHL,  // Shift left  (logical)
    LSHR, // Shift right (logical)
    ASHR, // Shift right (arithmetic)
    AND,
    OR,
    XOR,
    BinaryEnd,
    UnaryBegin,
    Assign,
    MINUS,
    F2I,
    I2F,
    AddSP,
    UnaryEnd,
    Move,
};

class Instrution : public User
{
    BasicBlock *parent;
    InstrutionEnum instr_type;

public:
    static std::unordered_map<InstrutionEnum, std::string> *_symbol_map;

    Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type);
    Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type, bool notPush);
    Instrution(BasicBlock *_BB, InstrutionEnum type, p_symbol_var p_var);
    Instrution(BasicBlock *_BB, InstrutionEnum type, Type *p_array, bool _ele);
    Instrution(BasicBlock *_BB, InstrutionEnum type, Type *_type, int is_copy);
    Instrution(BasicBlock *_BB, InstrutionEnum type, Type *_type, int is_copy, bool notPush);
    ~Instrution() override;

    void replaceInstr(BasicBlock *_BB, int pos);
    void insertInstr(BasicBlock *_BB, int pos);
    void removeInstr();
    void set_parent(BasicBlock *newf) { parent = newf; }

    InstrutionEnum get_Instrtype() { return instr_type; }

    BasicBlock *get_BB() { return parent; }

    bool isCall() { return instr_type == InstrutionEnum::Call; }
    bool isGEP() { return instr_type == InstrutionEnum::GEP; }
    bool isJmp() { return instr_type == InstrutionEnum::Jmp; }
    bool isBranch() { return instr_type == InstrutionEnum::Branch; }
    bool isPHINode() { return instr_type == InstrutionEnum::PHINode; }
    bool isReturn() { return instr_type == InstrutionEnum::Ret; }
    bool isAlloca() { return instr_type == InstrutionEnum::Alloca; }
    bool isLoad() { return instr_type == InstrutionEnum::Load; }
    bool isStore() { return instr_type == InstrutionEnum::Store; }
    bool isCmp() { return instr_type >= InstrutionEnum::CmpBegin && instr_type < InstrutionEnum::CmpEnd; }
    bool isBinary() { return instr_type >= InstrutionEnum::BinaryBegin && instr_type < InstrutionEnum::BinaryEnd; }
    bool isUnary() { return instr_type >= InstrutionEnum::UnaryBegin && instr_type < InstrutionEnum::UnaryEnd; }
    bool isIBinary() { return instr_type >= InstrutionEnum::IADD && instr_type <= InstrutionEnum::IMOD; }
    bool isMove() { return instr_type == InstrutionEnum::Move; }

    BasicBlock *get_parent() { return parent; }
    void setParent_F(BasicBlock *_BB) { parent = _BB; }

    InstrutionEnum get_Instype() { return instr_type; }
    bool is_commutative();
    Value *get_operand_at(int pos);
    void move_before_terminator();
    int get_pos_of_bb();

    void set_instrType(InstrutionEnum type) { instr_type = type; }
    void replaceAllUses(Value *RepVal);
    virtual void drop();

    virtual void print() override { assert(0); }
};
class Call : public Instrution // p_func
{
    Value *p_first_store; // Instr

public:
    Call(Value *_func, BasicBlock *_parent);

    Value *get_func() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_first() { return p_first_store; }

    void params_pushback(Value *_param);

    void print();
};
class GEP : public Instrution // p_addr p_offset
{
    bool is_element;

public:
    GEP(Value *_addr, Value *_offset, bool _elemet, BasicBlock *_parent);

    bool get_isele() { return is_element; }
    Value *get_addr() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_offset() { return (*this->get_value_list())[1]->get_val(); }

    void print();
};
class PHINode : public Instrution
{
    std::unordered_map<BasicBlock *, Edge *> *valueMap;

public:
    PHINode(BasicBlock *_BB, TypeEnum basic_type, bool notPush);
    PHINode(BasicBlock *_BB, Type *_type, bool notPush);
    std::unordered_map<BasicBlock *, Edge *> *get_valueMap() { return valueMap; }
    BasicBlock *get_edge_income_block(Edge *);
    ~PHINode();
    void addIncoming(Value *val, BasicBlock *BB);
    void eraseIncoming(BasicBlock *BB);
    void print();
    void drop();
};
class Ret : public Instrution // p_val
{

public:
    Ret(BasicBlock *_parent);
    Ret(Value *_val, BasicBlock *_parent);

    Value *get_val() { return this->get_value_list()->empty() ? nullptr : (*this->get_value_list())[0]->get_val(); }

    void print();
};
class Jmp : public Instrution // nextBB：nextBB是跳转指令所在块的使用者
{

public:
    Jmp(BasicBlock *_next, BasicBlock *_parent);

    BasicBlock *get_nextBB();

    void print();
};
class Branch : public Instrution // value: cond    // user: trurBB falseBB
{

public:
    Branch(Value *_cond, BasicBlock *_trueBB, BasicBlock *_falseBB, BasicBlock *_parent);

    Value *get_cond() { return (*this->get_value_list())[0]->get_val(); }
    BasicBlock *get_trueBB();
    BasicBlock *get_falseBB();

    void print();
};
class Load : public Instrution // p_addr
{
    bool is_stack_ptr;

public:
    Load(Value *p_val, bool _is_stack_ptr, BasicBlock *_parent);

    Value *get_addr() { return (*this->get_value_list())[0]->get_val(); }
    bool get_is_stack_prt() { return is_stack_ptr; }
    void print();
};
class Store : public Instrution // p_addr p_src
{
    bool is_stack_ptr;

public:
    Store(Value *_addr, Value *_src, bool _stack, BasicBlock *_BB);

    Value *get_addr() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_src() { return (*this->get_value_list())[1]->get_val(); }
    bool get_isptr() { return is_stack_ptr; }

    void print();
};
class Alloca : public Instrution
{

public:
    Alloca(BasicBlock *_perant, p_symbol_var p_var);
    Alloca(BasicBlock *_parent, TypeEnum type);
    Alloca(BasicBlock *_parent, Type *_type);
    Alloca(BasicBlock *_perant, Type *_type, int is_copy);

    void print();
};
class Cmp : public Instrution // p_scr1, p_src2
{

public:
    Cmp(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent);

    Value *get_src1() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_src2() { return (*this->get_value_list())[1]->get_val(); }
    bool isCond();
    void print();
};
class Binary : public Instrution
{

public:
    Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent);
    Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent, bool is_ele);

    Value *get_src1() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_src2() { return (*this->get_value_list())[1]->get_val(); }

    void print();
};

class Unary : public Instrution
{

public:
    Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent);
    Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent, bool notPush);

    Value *get_src() { return (*this->get_value_list())[0]->get_val(); }

    void print();
};

class Assign : public Unary
{
public:
    Assign(InstrutionEnum type, Value *_src1, BasicBlock *_parent)
        : Unary(type, _src1, _parent) {}
    Assign(InstrutionEnum type, Value *_src1, BasicBlock *_parent, bool notPush)
        : Unary(type, _src1, _parent, notPush) {}
    void print();
};

class Move : public Instrution
{
public:
    Move(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent);

    Value *get_src1() { return (*this->get_value_list())[0]->get_val(); }
    Value *get_src2() { return (*this->get_value_list())[1]->get_val(); }

    void print();
};
