#pragma once
#include <backend/rv/RVstand.hpp>
#include <vector>
#include <string>
class RVEdge;
class RVValue
{
    std::vector<RVEdge *> value_list;
    std::vector<RVEdge *> user_list;

public:
    std::vector<RVEdge *> get_value_list() { return value_list; }
    std::vector<RVEdge *> get_user_list() { return user_list; }
    void value_list_push_back(RVEdge *e) { value_list.push_back(e); }
    void user_list_push_back(RVEdge *e) { user_list.push_back(e); }
    virtual void print() {}
    virtual ~RVValue() {}
};
class RVEdge
{
    RVValue *val;
    RVValue *user;

public:
    RVEdge(RVValue *v, RVValue *u);
    RVValue *get_user() { return user; }
    RVValue *get_val() { return val; }
};

class RVLable : public RVValue
{
    std::string name;

public:
    RVLable(std::string na) : name(na) {}
    std::string get_name() { return name; }
};
class RVOperand : public RVValue
{
public:
    virtual void print() {}
    virtual ~RVOperand() {}
};
class RVAddr : public RVOperand
{
    RVLable *label;

public:
    RVAddr(RVLable *l) : label(l) {}
    void print();
    ~RVAddr() {}
};
class RVReg_Reg : public RVOperand
{
};
class RVReg : public RVOperand
{
    int rno;

public:
    RVReg(int r) : rno(r) {}
    int get_no() { return rno; }
    bool is_r_reg() { return rv_is_r_reg(rno); }
    bool is_f_reg() { return rv_is_f_reg(rno); }
    void print();
    ~RVReg();
};
class RVReg_d : public RVReg
{

public:
    RVReg_d(int r) : RVReg(r) {}
    ~RVReg_d() {}
};
class RVReg_off : public RVOperand
{
    int rno;
    int offset;

public:
    RVReg_off(int r, int off) : rno(r), offset(off) {}
    int get_off() { return offset; }
    void print();
    ~RVReg_off();
};
class RVImme : public RVOperand
{
    int imme;

public:
    RVImme(int i) : imme(i) {}
    int get_imme() { return imme; }
    void print();
    ~RVImme();
};
class RVImmef : public RVOperand
{
    float imme;

public:
    RVImmef(float i) : imme(i) {}
    float get_immef() { return imme; }
    void print();
    ~RVImmef();
};
class RVInstr : public RVValue
{
    RVENUM instr_enum;
    // RVextend extend_enum;
    RVcond cond_enum = RVcond::nill;
    // std::vector<RVfloat> float_enums;
    std::vector<RVOperand *> ops;

public:
    RVInstr(RVENUM i) : instr_enum(i) {};
    // RVInstr(RVENUM i, RVextend e) : instr_enum(i), extend_enum(e) {};
    RVInstr(RVENUM i, RVcond c) : instr_enum(i), cond_enum(c) {};
    // RVInstr(RVENUM i, RVcond c, RVfloat f);
    // RVInstr(RVENUM i, RVcond c, RVfloat f1, RVfloat f2);
    RVENUM get_enum() { return instr_enum; }
    void set_cond(RVcond c) { cond_enum = c; }
    void ops_push_back(RVOperand *o) { ops.push_back(o); }
    std::vector<RVOperand *> get_ops() { return ops; }
    bool is_binary() { return instr_enum > RVENUM::rv_binary_begin && instr_enum < RVENUM::rv_binary_end; }
    RVOperand *get_op_at(int p) { return ops[p]; }
    void replace_op_with(RVOperand *o, RVOperand *newo);
    void replace_op_with_and_delete(RVOperand *o, RVOperand *newo);

    void print();
    void print_enum();
    ~RVInstr();
};
class RVconstLable : public RVLable
{
    float word;

public:
    RVconstLable(std::string na, float w) : RVLable(na), word(w) {}
    void print();
};
class RVBlock : public RVLable
{
    std::vector<RVInstr *> instrs;

public:
    RVBlock(std::string nam) : RVLable(nam) {}
    void instr_push_back(RVInstr *i) { instrs.push_back(i); }
    void instr_insert_before(RVInstr *pos, RVInstr *in);
    void instr_insert_after(RVInstr *pos, RVInstr *in);
    RVInstr *get_last_instr() { return instrs.back(); }
    std::vector<RVInstr *> get_instrs() { return instrs; }
    void print();

    ~RVBlock();
};
class RVFunc : public RVLable
{
    int is_external = 0;
    std::vector<RVBlock *> blocks;
    std::vector<RVconstLable *> constlables;

public:
    RVFunc(std::string nam) : RVLable(nam) {}
    void set_external() { is_external = 1; }
    bool is_exter() { return is_external; }
    void blocks_push_back(RVBlock *b) { blocks.push_back(b); }
    void constlable_push_backe(RVconstLable *c) { constlables.push_back(c); }
    std::vector<RVBlock *> get_blocks() { return blocks; }
    std::vector<RVconstLable *> get_constlables() { return constlables; }
    void print();
    ~RVFunc();
};
class RVGlobalVariable : public RVLable
{
    std::vector<uint32_t> words;
    int space; // zeros+words

public:
    RVGlobalVariable(std::string nam) : RVLable(nam) {}
    void words_push_back(uint32_t u) { words.push_back(u); }
    void set_space(int i) { space = i; }
    void print();
};

class RVModule
{
    std::vector<RVFunc *> funcs;
    std::vector<RVGlobalVariable *> globals;
    std::string infile;
    std::string outfile;

public:
    RVModule(std::string inf, std::string outf) : infile(inf), outfile(outf) {}
    void funcs_push_back(RVFunc *f) { funcs.push_back(f); }
    void globals_push_back(RVGlobalVariable *g) { globals.push_back(g); }
    std::vector<RVFunc *> get_funcs() { return funcs; }
    void print(int test);
    ~RVModule();
};
template <typename T>
bool is_a(RVValue *p_val)
{
    return dynamic_cast<T *>(p_val) != nullptr;
}