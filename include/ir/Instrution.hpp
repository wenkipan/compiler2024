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
    UnaryEnd,
};

class Instrution : public User
{
    BasicBlock *parent;
    InstrutionEnum instr_type;

public:
    Instrution(BasicBlock *_BB, InstrutionEnum type, TypeEnum basic_type);
    Instrution(/* args */);
    ~Instrution();
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
};
class GEP : public Instrution
{
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
    Jmp();
};
class Branch : public Instrution
{
    Value *cond;
    BasicBlock *trueBB;
    BasicBlock *falseBB;

public:
    Branch();
};
class Load : public Instrution
{
};
class Store : public Instrution
{
    Value *p_addr;
    Value *p_src;

public:
    Store(Value *_addr, Value *_src, BasicBlock *_BB);
};
class Alloc : public Instrution
{
};
class Cmp : public Instrution
{
};
class Binary : public Instrution
{
};
class Unary : public Instrution
{
};