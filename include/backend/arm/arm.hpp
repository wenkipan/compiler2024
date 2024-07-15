#include <string>
#include <lir/ArmStand.hpp>
#include <vector>

enum class ARMENUM
{
    arm_binary_begin,
    arm_add,
    arm_sub,
    arm_mul,
    arm_sdiv,
    arm_rsb,
    arm_lsl,
    arm_lsr,
    arm_asr,
    arm_and,
    arm_eor,
    arm_orr,
    arm_orn,
    arm_vadd_f32, // float
    arm_vsub_f32,
    arm_vmul_f32,
    arm_vdiv_f32,
    arm_vand,
    arm_binary_end,

    arm_cmp_begin,
    arm_cmp,
    arm_cmn,
    arm_tst,
    arm_teq,
    arm_vcmp_f32,
    // magic code:
    //   vmrs APSR_nzcv, FPSCR\n
    arm_vtst,
    arm_cmp_end,

    arm_mem_begin,
    arm_str,
    arm_ldr,
    arm_vstr_32, // float
    arm_vldr_32,
    arm_mem_end,

    arm_mov_begin,
    arm_moveq,
    arm_movne,
    arm_movge,
    arm_movgt,
    arm_movle,
    arm_movlt,
    arm_mov,
    arm_mov_32,
    arm_movw,
    arm_movt,
    arm_mvn,
    arm_vmov,
    arm_vmov_f32,
    arm_vneg_f32,
    arm_mov_end,

    arm_push_pop_begin,
    arm_push,
    arm_pop,
    arm_vpush, // float
    arm_vpop,
    arm_push_pop_end,

    arm_vcvt_begin,
    arm_vcvt_s32_f32,
    arm_vcvt_f32_s32,
    arm_vcvt_end,

    // jump actually
    arm_jump_begin,
    arm_beq,
    arm_bne,
    arm_bge,
    arm_bgt,
    arm_ble,
    arm_blt,
    arm_bal,

    arm_b,
    arm_bx,
    arm_bl,
    arm_blx,
    arm_jump_end,

};
enum class ARMCOND
{
    eq,
    ne,
    ge,
    gt,
    le,
    lt,
    al,
};
/*
r0 	易失的 	参数寄存器、结果寄存器、临时寄存器 1
r1 	易失的 	参数寄存器、结果寄存器、临时寄存器 2
r2 	易失的 	参数寄存器、临时寄存器 3
r3 	易失的 	参数寄存器、临时寄存器 4
r4 	非易失性的
r5 	非易失性的
r6 	非易失性的
r7 	非易失性的
r8 	非易失性的
r9 	非易失性的
r10 	非易失性的
r11 	非易失性的 	帧指针
r12 	易失的 	过程内部调用临时寄存器
r13 (SP) 	非易失性的 	堆栈指针
r14 (LR) 	非易失性的 	链接寄存器
r15 (PC) 	非易失性的 	程序计数器
*/
/*
s0-s3 	d0-d1 	q0 	易失的 	参数寄存器、结果寄存器、临时寄存器
s4-s7 	d2-d3 	q1 	易失的 	参数寄存器、临时寄存器
s8-s11 	d4-d5 	q2 	易失的 	参数寄存器、临时寄存器
s12-s15 	d6-d7 	q3 	易失的 	参数寄存器、临时寄存器
s16-s19 	d8-d9 	q4 	非易失性的
s20-s23 	d10-d11 	q5 	非易失性的
s24-s27 	d12-d13 	q6 	非易失性的
s28-s31 	d14-d15 	q7 	非易失性的
*/
// fp r11(gcc) or r7(llvm)
#define R0 0
#define RTMP 12 // also know as ip in gcc
#define SP 13
#define LR 14
#define PC 15
#define S0 16
std::string printENUM(ARMENUM ARMENUM);

class ArmEdge;
class ArmValue
{
public:
    std::vector<ArmEdge *> *value_list;
    std::vector<ArmEdge *> *user_list;
    std::vector<ArmEdge *> *get_value_list() { return value_list; }
    std::vector<ArmEdge *> *get_user_list() { return user_list; }
    void value_list_push_back(ArmEdge *e) { value_list->push_back(e); }
    void user_list_push_back(ArmEdge *e) { user_list->push_back(e); }
    // virtual void print() { assert(0); }
    ArmValue();
    virtual ~ArmValue();
};
class ArmEdge
{
public:
    ArmValue *value;
    ArmValue *user;
    ArmEdge(ArmValue *val, ArmValue *u);
    void drop();
};

// template <typename T>
// bool is_a(ArmValue *p_val);
template <typename T>
bool is_a(ArmValue *p_val)
{
    return dynamic_cast<T *>(p_val) != nullptr;
}
class ArmOperand : public ArmValue
{
public:
    // ArmOperand();
    virtual void print() { assert(0); }
    virtual ~ArmOperand() {}
};

class ArmAddr : public ArmOperand
{
    ArmValue *addr;

public:
    ArmAddr(ArmValue *add) : addr(add) {}
    ArmValue *get_addr() { return addr; }
    void set_addr(ArmValue *add) { addr = add; }
    void print();
    ~ArmAddr() {}
};
class ArmReg : public ArmOperand
{
    int regno;
    int offset = 0;
    bool is_addr = 0;

public:
    ArmReg(int _regno) : regno(_regno) { assert(_regno >= 0); }
    ArmReg(int _re, int off) : regno(_re), offset(off), is_addr(1) {}
    bool is_s_reg() { return regno > 15 && regno <= 15 + 32; }
    bool is_r_reg() { return 0 <= regno && regno <= 15; }
    int get_offset() { return offset; }
    void print();
    ~ArmReg() {}
};
class ArmImme : public ArmOperand
{
    uint32_t imme_int;
    // float imme_float;

public:
    ArmImme(uint32_t _imme) : imme_int(_imme) {}
    ArmImme(int i) : imme_int((uint32_t)i) {}
    // ArmImme(float _imme) : imme_float(_imme) {}
    void print();
    ~ArmImme() {}
};
class ArmImmef : public ArmOperand
{
    float imme_float;

public:
    ArmImmef(float f) : imme_float(f) {}
    void print();
    ~ArmImmef() {}
};
class ArmInstr : public ArmValue
{
    ARMENUM armenum;
    // ARMCOND *cond = nullptr;
    std::vector<ArmOperand *> ops;

public:
    ArmInstr(ARMENUM ae) : armenum(ae) {}
    void ops_push_back(ArmOperand *r) { ops.push_back(r); }
    void set_enum(ARMENUM ae) { armenum = ae; }
    ARMENUM get_enum() { return armenum; }
    ArmOperand *get_op_at(int pos) { return ops[pos]; }
    void print();
    bool is_arm_push_pop() { return ARMENUM::arm_push_pop_begin < armenum && armenum < ARMENUM::arm_push_pop_end; }
    bool is_b() { return ARMENUM::arm_jump_begin < armenum && armenum < ARMENUM::arm_jump_end; }
    std::vector<ArmOperand *> get_ops() { return ops; }
    ~ArmInstr();

public:
};
class ArmFunc;
class ArmBlock : public ArmValue
{
    std::string name;
    std::vector<ArmInstr *> instrs;
    ArmFunc *parent = nullptr;

public:
    void instrs_push_back(ArmInstr *i)
    {
        instrs.push_back(i);
        i->print();
    }
    void instrs_insert_before(int pos, ArmInstr *i)
    {
        instrs.insert(instrs.begin() + pos, i);
    }
    ArmBlock(std::string na, ArmFunc *f) : name(na), parent(f) {}
    ArmFunc *get_parent() { return parent; }
    void set_name(std::string na) { name = na; }
    std::string get_name() { return name; }
    std::vector<ArmInstr *> get_instrs() { return instrs; }
    ArmInstr *get_last_instr() { return instrs.back(); }
    int find_instr_pos(ArmInstr *i);
    void print();
    void drop();
    ~ArmBlock();
};
class Armconstlable : public ArmValue
{
    std::string name;
    uint32_t word;
    ArmFunc *parent = nullptr;

public:
    Armconstlable(std::string na, uint32_t w, ArmFunc *f) : name(na), word(w), parent(f) {}
    Armconstlable(std::string na, int w, ArmFunc *f) : name(na), word((uint32_t)w), parent(f) {}
    ArmFunc *gen_parent() { return parent; }
    void print();
};
class ArmGlobal : public ArmValue
{
};
class ArmFunc : public ArmGlobal
{
    std::string name;
    std::vector<ArmBlock *> blocks;
    std::vector<Armconstlable *> constantlables;
    bool external = 0;

public:
    void blocks_push_back(ArmBlock *ab) { blocks.push_back(ab); }
    void constlable_push_back(Armconstlable *b) { constantlables.push_back(b); }

    void set_name(std::string na) { name = na; }
    std::string get_name() { return name; }
    std::vector<ArmBlock *> get_blocks() { return blocks; }
    std::vector<Armconstlable *> get_constlable() { return constantlables; }
    void set_external() { external = 1; }
    bool is_external() { return external; }
    void print();
    ~ArmFunc();
};
class ArmGlobalVariable : public ArmGlobal
{
    std::string name;
    std::vector<uint32_t> words;
    int space;

public:
    ArmGlobalVariable(std::string _name) : name(_name) {}
    void words_push_back(uint32_t i) { words.push_back(i); }
    void set_space(int s) { space = s; }
    std::string get_name() { return name; }
    void print();
};

class ArmModule
{
    std::vector<ArmGlobalVariable *> globals;
    std::vector<ArmFunc *> funcs;
    std::string infile;
    std::string outfile;

public:
    // ArmModule();
    ArmModule(std::string inf, std::string outf) : infile(inf), outfile(outf) {}
    void gvs_push_back(ArmGlobalVariable *ab) { globals.push_back(ab); }
    void funcs_push_back(ArmFunc *f) { funcs.push_back(f); }
    std::vector<ArmFunc *> get_funcs() { return funcs; }
    void print(int test);
    ~ArmModule();
};

std::vector<ArmBlock *> RPO(ArmFunc *f);