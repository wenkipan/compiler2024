#pragma once

#include <backend/arm/arm.hpp>
#include <ir/ir.hpp>
#include <ir_opt/SSARegisterAlloc.hpp>

class ArmGen
{
    std::unordered_map<Value *, ArmValue *> val2val_map;
    std::unordered_map<BasicBlock *, ArmBlock *> bb_ab_map;
    // TODO::it should go with spinfo
    std::unordered_map<Value *, int> alloc_and_param_offset_map;
    ArmModule *arm_module;
    // sp info
    class SPinfo
    {
    public:
        int sp_sub_offset;
        std::vector<int> used_r_reg;
        std::vector<int> used_s_reg;
        SPinfo(int a, std::vector<int> r, std::vector<int> f)
        {
            sp_sub_offset = a;
            used_r_reg = r;
            used_s_reg = f;
        }
    };
    std::unordered_map<Function *, SPinfo> spinfomap;
    std::unordered_map<Function *, SSARegisterAlloc *> ssaramap;
    SSARegisterAlloc *ssara;

public:
    ArmModule *get_arm() { return arm_module; }
    void run(Module *m);
    // support
    ArmBlock *get_ab(BasicBlock *b);
    ArmValue *getArmval(Value *v);
    void set_offset(Value *, int);
    int get_offset(Value *addr);
    ArmOperand *get_op(Value *i, ArmBlock *b);
    void gen_mov_imme32(int Rno, int imme, ArmBlock *bb);
    ArmOperand *gen_legal_imme(int imme, ArmBlock *b);
    // ArmInstr *gen_mov_imme(int dst, int imme, ArmBlock *b);
    void gen_mov_imme(int dst, int imme, ArmBlock *b);
    void gen_mov(Value *dst, Value *src, ArmBlock *bb);
    ArmReg *gen_sp_and_offset_op(int offset, ArmBlock *b);
    // init with name
    void init(Module *m);
    ArmFunc *init_func(Function *f);
    // gen offsets
    void gen_sp_sub_and_offset_for_alloc_and_param(Function *f, ArmBlock *b);
    // restore sp
    void gen_sp_add_and_pop(Function *f, ArmBlock *b);
    //  gen instrs
    void gen_func(Function *f);
    ArmModule *gen_module(Module *m);
    ArmBlock *gen_block(BasicBlock *b);
    void gen_instr(Instrution *i, ArmBlock *b);
    void gen_binary(Instrution *i, ArmBlock *b);
    void gen_unary(Instrution *i, ArmBlock *b);
    void gen_call(Instrution *i, ArmBlock *b);
    void gen_ret(Instrution *i, ArmBlock *b);
    void gen_branch(Instrution *i, ArmBlock *b);
    void gen_jmp(Instrution *i, ArmBlock *b);
    void gen_load(Instrution *i, ArmBlock *b);
    void gen_load_GV_addr(Value *addr, ArmBlock *b);
    void gen_store(Instrution *i, ArmBlock *b);
    void gen_cmp(Instrution *i, ArmBlock *b);
    ~ArmGen()
    {
        for (auto kv : ssaramap)
            delete kv.second;
    }
};

/*
|a6|
|a5|  params
|a4|
------
|push|
|push|   pushedsize
|vpush|
-------------------------------------------- sp
|alloc|
|alloc|
|alloc|
|alloc|   allocsize
|alloc|
|alloc|
-------                                   sp_sub_offset
|calleea6|
|calleea5|  max callee_param_sub4
|calleea4|
-------------------------------------------- sp sub x
*/