#pragma once
#include "ir_opt/SSARegisterAlloc.hpp"
#include <backend/rv/RV.hpp>

#include <ir/ir.hpp>

class funcInfo
{
public:
    std::vector<Param *> paramstostack;

    SSARegisterAlloc *ssara = nullptr;
    RVFunc *rv_f = nullptr;
    Function *link_f;

    int sp_sub_offset;
    int allocsize;
    int pushedsize;
    int max_pushed_callee_param_and_resume;
    std::vector<int> regs_need_resume;
};

class RVGen
{
    RVModule *rvm;
    std::unordered_map<Function *, funcInfo *> infomap;

    std::unordered_map<Value *, RVValue *> val2valmap;
    RVValue *getrvval(Value *a);

    std::unordered_map<Value *, int> alloc_and_param_offset_map;
    void set_offset(Value *i, int off);
    int get_offset(Value *i);

    RVBlock *cur_b = nullptr;
    SSARegisterAlloc *ssara = nullptr;

public:
    RVModule *get_modu() { return rvm; }
    void run(Module *m);
    void init(Module *m);
    RVFunc *init_f(Function *f);
    void gen_fnc(Function *f);
    void gen_instr(Instrution *i);
    void gen_sp_sub_and_offset_for_alloc_and_param(Function *f);
    int find_max_pushed_callee_param(Function *f);
    int find_all_alloc_size(Function *f);
    void gen_sp_add_and_pop(Function *f);

    void gen_load_GV_addr(int rno, Value *addr);
    RVReg_off *get_op_addr(Value *i);
    RVOperand *get_op(Value *v, RVInstr *pos);
    RVOperand *get_op(Value *i, bool call_use, RVInstr *pos);
    void gen_binary(Instrution *i);
    void gen_unary(Instrution *i);
    void gen_call(Instrution *i);
    void gen_call_before(Instrution *i);
    void gen_call_after(Instrution *i);
    void gen_ret(Instrution *i);
    void gen_branch(Instrution *i);
    void gen_jmp(Instrution *i);
    void gen_cmp(Instrution *i);
    void gen_load(Instrution *i);
    void gen_store(Instrution *i);
    RVInstr *gen_mov(Value *i, Value *i2);
    ~RVGen()
    {
        for (auto kv : infomap)
        {
            delete kv.second->ssara;
            delete kv.second;
        }
    }
};
