#pragma once

#include <ir/User.hpp>

class BasicBlock;

// maybe change to Binarybegin  Binaryend
enum class InstrutionEnum
{
    Call,
    GEP,
    Jmp,
    Branch,
    PHINode,
    Return,
    Load,
    Store,
    Alloc,
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
    // bitwise
    SHL,  // Shift left  (logical)
    LSHR, // Shift right (logical)
    ASHR, // Shift right (arithmetic)
    AND,
    OR,
    XOR,
    BinaryEnd,
    UnaryBegin,
    MINUS,
    F2I,
    I2F,
    AddSP,
    UnaryEnd
};

class Instrution : public User
{
    BasicBlock *parent;
    InstrutionEnum instr_type;

public:
    Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type);
    Instrution(BasicBlock *_BB, InstrutionEnum type, p_symbol_var p_var);

    BasicBlock *get_BB() { return parent; }

    bool isCall() { return instr_type == InstrutionEnum::Call; }
    bool isGEP() { return instr_type == InstrutionEnum::GEP; }
    bool isJmp() { return instr_type == InstrutionEnum::Jmp; }
    bool isBranch() { return instr_type == InstrutionEnum::Branch; }
    bool isPHINode() { return instr_type == InstrutionEnum::PHINode; }
    bool isReturn() { return instr_type == InstrutionEnum::Return; }
    bool isLoad() { return instr_type == InstrutionEnum::Load; }
    bool isStore() { return instr_type == InstrutionEnum::Store; }
    bool isCmp() { return instr_type >= InstrutionEnum::CmpBegin && instr_type < InstrutionEnum::CmpEnd; }
    bool isBinary() { return instr_type >= InstrutionEnum::BinaryBegin && instr_type < InstrutionEnum::BinaryEnd; }
    bool isUnary() { return instr_type >= InstrutionEnum::UnaryBegin && instr_type < InstrutionEnum::UnaryEnd; }
};
class Call : public Instrution
{
    Value *p_func;
    Value *p_first_store; // Instr
    std::vector<Value *> params;

public:
    Call(Value *_func, BasicBlock *_parent);

    Value *get_func() { return p_func; }
    Value *get_first() { return p_first_store; }

    void params_pushback(Value *_param);
};
class GEP : public Instrution
{
    Value *p_addr, *p_offset;

public:
    GEP(Value *_addr, Value *_offset, BasicBlock *_parent);

    Value *get_addr() { return p_addr; }
    Value *get_offset() { return p_offset; }
};
class PHINode : public Instrution
{
};
class Return : public Instrution
{
};
class Jmp : public Instrution
{
    BasicBlock *nextBB;

public:
    Jmp(BasicBlock *_parent, BasicBlock *_next);

    BasicBlock *get_nextBB() { return nextBB; }
};
class Branch : public Instrution
{
    Value *cond;
    BasicBlock *trueBB;
    BasicBlock *falseBB;

public:
    Branch(Value *_cond, BasicBlock *_trueBB, BasicBlock *_falseBB, BasicBlock *_parent);

    Value *get_cond() { return cond; }
    BasicBlock *get_trueBB() { return trueBB; }
    BasicBlock *get_falseBB() { return falseBB; }
};
class Load : public Instrution
{
    Value *p_addr;
    bool is_stack_ptr;

public:
    Load(Value *p_val, bool _is_stack_ptr, BasicBlock *_parent);
};
class Store : public Instrution
{
    Value *p_addr;
    Value *p_src;
    bool is_stack_ptr;

public:
    Store(Value *_addr, Value *_src, bool _stack, BasicBlock *_BB);

    Value *get_addr() { return p_addr; }
    Value *get_src() { return p_src; }
    bool get_isptr() { return is_stack_ptr; }
};
class Alloc : public Instrution
{

public:
    Alloc(BasicBlock *_perant, p_symbol_var p_var);
    Alloc(BasicBlock *_parent, TypeEnum type);
};
class Cmp : public Instrution
{
    Value *p_src1, *p_src2;

public:
    Cmp(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent);

    Value *get_src1() { return p_src1; }
    Value *get_src2() { return p_src2; }
};
class Binary : public Instrution
{
    Value *p_src1, *p_src2;

public:
    Binary(InstrutionEnum type, Value *_src1, Value *_src2, BasicBlock *_parent);

    Value *get_src1() { return p_src1; }
    Value *get_src2() { return p_src2; }
};
class Unary : public Instrution
{
    Value *p_src;

public:
    Unary(InstrutionEnum type, Value *_src1, BasicBlock *_parent);

    Value *get_src() { return p_src; }
};