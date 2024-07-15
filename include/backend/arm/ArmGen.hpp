#pragma once

#include <backend/arm/arm.hpp>
#include <ir/ir.hpp>
#include <ir_opt/SSARegisterAlloc.hpp>

class ArmGen
{
    std::unordered_map<Value *, ArmValue *> val2val_map;
    std::unordered_map<BasicBlock *, ArmBlock *> bb_ab_map;
    std::unordered_map<Value *, int> alloc_and_param_offset_map;
    ArmModule *arm_module;
    // sp info
    class SPinfo
    {
    public:
        int sp_sub_offset;
        std::vector<int> used_reg;
        SPinfo(int a, std::vector<int> r)
        {
            sp_sub_offset = a;
            used_reg = r;
        }
    };
    std::unordered_map<Function *, SPinfo> spinfomap;
    std::unordered_map<Function *, SSARegisterAlloc *> ssaramap;
    SSARegisterAlloc *ssara;
    class paramsinfo
    {
    public:
        std::vector<Param *> paramstostack;
        paramsinfo(std::vector<Param *> s) : paramstostack(s) {}
        int allcount() { return paramstostack.size(); }
    };
    std::unordered_map<Function *, paramsinfo> paraminfomap;

public:
    ArmModule *get_arm() { return arm_module; }
    void run(Module *m);
    // support
    ArmBlock *get_ab(BasicBlock *b);
    ArmValue *getArmval(Value *v);
    void set_offset(Value *, int);
    int get_offset(Value *addr);
    int find_max_pushed_callee_param(Function *f);
    ArmOperand *get_op(Value *i, ArmBlock *b, int must_reg);
    ArmOperand *get_op_addr(Value *i, ArmBlock *b);
    void gen_mov_imme32(int Rno, int imme, ArmBlock *bb);
    ArmOperand *gen_legal_imme(int imme, ArmBlock *b);
    void gen_mov_imme(int dst, int imme, ArmBlock *b);
    ArmInstr *gen_mov(Value *dst, Value *src, ArmBlock *bb);
    ArmReg *gen_sp_and_offset_op(int offset, ArmBlock *b);
    int find_all_alloc_size(Function *f);
    void gen_push_or_pop(ARMENUM ae, std::vector<int> v, ArmBlock *b, int pos);

    // init with name
    void init(Module *m);
    ArmFunc *init_func(Function *f);
    void gen_sp_sub_and_offset_for_alloc_and_param(Function *f, ArmBlock *b);
    void gen_sp_add_and_pop(Function *f, ArmBlock *b);
    //  gen instrs
    void gen_func(Function *f);
    void gen_instr(Instrution *i, ArmBlock *b);
    void gen_binary(Instrution *i, ArmBlock *b);
    void gen_unary(Instrution *i, ArmBlock *b);
    void gen_call_before(Instrution *i, ArmBlock *b);
    void gen_call_after(Instrution *i, ArmBlock *b);
    void gen_call(Instrution *i, ArmBlock *b);
    void gen_ret(Instrution *i, ArmBlock *b);
    void gen_branch(Instrution *i, ArmBlock *b);
    void gen_jmp(Instrution *i, ArmBlock *b);
    void gen_load(Instrution *i, ArmBlock *b);
    void gen_load_GV_addr(int no, Value *addr, ArmBlock *b);
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
|stillalive1|
|stillalive2|
----------            max_pushed_callee_param
|calleea6|
|calleea5|
|calleea4|
-------------------------------------------- sp sub x
*/