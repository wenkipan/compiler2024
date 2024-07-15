#include <algorithm>
#include <backend/arm/arm.hpp>
#include <ostream>
#include <string>
#include <iostream>

#include <stdio.h>
#include <unistd.h>

#define ENUM_TO_STRING_CASE(enumeration, value) \
    case enumeration::value:                    \
        print_enum = #value;                    \
        return print_enum.substr(4, print_enum.size() - 4);
std::string printENUM(ARMENUM ARMENUM)
{
    std::string print_enum;
    switch (ARMENUM)
    {
        ENUM_TO_STRING_CASE(ARMENUM, arm_add)
        ENUM_TO_STRING_CASE(ARMENUM, arm_sub)
        ENUM_TO_STRING_CASE(ARMENUM, arm_mul)
        ENUM_TO_STRING_CASE(ARMENUM, arm_sdiv)
        ENUM_TO_STRING_CASE(ARMENUM, arm_rsb)
        ENUM_TO_STRING_CASE(ARMENUM, arm_lsl)
        ENUM_TO_STRING_CASE(ARMENUM, arm_lsr)
        ENUM_TO_STRING_CASE(ARMENUM, arm_asr)
        ENUM_TO_STRING_CASE(ARMENUM, arm_and)
        ENUM_TO_STRING_CASE(ARMENUM, arm_eor)
        ENUM_TO_STRING_CASE(ARMENUM, arm_orr)
        ENUM_TO_STRING_CASE(ARMENUM, arm_orn)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vadd_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vsub_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vmul_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vdiv_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vand)

        ENUM_TO_STRING_CASE(ARMENUM, arm_cmp)
        ENUM_TO_STRING_CASE(ARMENUM, arm_cmn)
        ENUM_TO_STRING_CASE(ARMENUM, arm_tst)
        ENUM_TO_STRING_CASE(ARMENUM, arm_teq)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vcmp_f32) // magic code:
                                                   //   vmrs APSR_nzcv, FPSCR\n
        ENUM_TO_STRING_CASE(ARMENUM, arm_vtst)

        ENUM_TO_STRING_CASE(ARMENUM, arm_str)
        ENUM_TO_STRING_CASE(ARMENUM, arm_ldr)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vstr_32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vldr_32)

        ENUM_TO_STRING_CASE(ARMENUM, arm_moveq)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movne)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movge)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movgt)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movle)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movlt)
        ENUM_TO_STRING_CASE(ARMENUM, arm_mov)
        ENUM_TO_STRING_CASE(ARMENUM, arm_mov_32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movw)
        ENUM_TO_STRING_CASE(ARMENUM, arm_movt)
        ENUM_TO_STRING_CASE(ARMENUM, arm_mvn)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vmov)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vmov_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vneg_f32)

        ENUM_TO_STRING_CASE(ARMENUM, arm_push)
        ENUM_TO_STRING_CASE(ARMENUM, arm_pop)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vpush)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vpop)

        ENUM_TO_STRING_CASE(ARMENUM, arm_vcvt_s32_f32)
        ENUM_TO_STRING_CASE(ARMENUM, arm_vcvt_f32_s32)

        ENUM_TO_STRING_CASE(ARMENUM, arm_beq)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bne)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bge)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bgt)
        ENUM_TO_STRING_CASE(ARMENUM, arm_ble)
        ENUM_TO_STRING_CASE(ARMENUM, arm_blt)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bal)

        ENUM_TO_STRING_CASE(ARMENUM, arm_b)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bx)
        ENUM_TO_STRING_CASE(ARMENUM, arm_bl)
        ENUM_TO_STRING_CASE(ARMENUM, arm_blx)
    default:
        return "Unknown";
    }
}
ArmValue::ArmValue()
{
    value_list = new std::vector<ArmEdge *>;
    user_list = new std::vector<ArmEdge *>;
}
ArmValue::~ArmValue()
{
    delete value_list;
    delete user_list;
}
ArmEdge::ArmEdge(ArmValue *val, ArmValue *u)
    : value(val), user(u)
{
    val->get_user_list()->push_back(this);
    u->get_value_list()->push_back(this);
}
void ArmEdge::drop()
{
    auto it = std::find(value->get_user_list()->begin(), value->get_user_list()->end(), this);
    value->get_user_list()->erase(it);
    it = std::find(user->get_value_list()->begin(), user->get_value_list()->end(), this);
    user->get_value_list()->erase(it);
    delete this;
}
void ArmBlock::drop()
{
    for (auto val : *value_list)
    {
        ArmBlock *pred = (ArmBlock *)val->value;
        auto it = std::find(pred->get_user_list()->begin(), pred->get_user_list()->end(), val);
        pred->get_user_list()->erase(it);
    }
    for (auto user : *user_list)
    {
        ArmBlock *succ = (ArmBlock *)user->value;
        auto it = std::find(succ->get_value_list()->begin(), succ->get_value_list()->end(), user);
        succ->get_user_list()->erase(it);
    }
    for (auto i : instrs)
        delete i;
    delete this;
}

static void inline visit(ArmBlock *bb, std::set<ArmBlock *> &visited, std::vector<ArmBlock *> &order)
{
    if (visited.find(bb) != visited.end())
        return;

    visited.emplace(bb);
    for (auto succ : *bb->get_user_list())
    {
        visit((ArmBlock *)succ->user, visited, order);
    }
    order.push_back(bb);
}
std::vector<ArmBlock *> RPO(ArmFunc *f)
{
    std::vector<ArmBlock *> order;
    std::set<ArmBlock *> visited;
    visit(f->get_blocks().front(), visited, order);
    std::reverse(order.begin(), order.end());
    return order;
}
int ArmBlock::find_instr_pos(ArmInstr *i)
{
    int pos = 0;
    for (auto it : this->get_instrs())
    {
        if (it == i)
            return pos;
        pos++;
    }
    assert(0);
}

// print
void ArmModule::print(int test)
{
    int saved_stdout = dup(STDOUT_FILENO);
    if (!test)
        freopen(outfile.c_str(), "w", stdout);

    std::cout << ".file \"" << infile << "\"" << std::endl;
    std::cout << "   .arch armv7ve" << std::endl;
    std::cout << "   .arm" << std::endl;
    std::cout << "   .fpu neon-vfpv4" << std::endl
              << std::endl;

    std::cout << "   .section .data " << std::endl;
    for (auto g : globals)
    {
        g->print();
    }
    std::cout << std::endl;
    std::cout << "   .section .text" << std::endl;
    for (auto f : funcs)
    {
        if (f->is_external())
            continue;
        f->print();
    }

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}
void ArmGlobalVariable::print()
{
    std::cout << name << ":" << std::endl;
    for (auto word : words)
        std::cout << "   .word " << word << std::endl;
    if (words.size() != 1)
        std::cout << "   .space " << space << std::endl;
}
void ArmFunc::print()
{
    std::cout << name << ":" << std::endl;
    // use with print
    // auto BBs = RPO(this);
    // for (int i = 0; i < BBs.size() - 1; i++)
    // {
    //     auto l = BBs[i]->get_last_instr();
    //     if (l->is_b() && ((ArmAddr *)l->get_op_at(0))->get_addr() == BBs[i + 1])
    //     {
    //         BBs[i]->get_instrs().erase(BBs[i]->get_instrs().end() - 1);
    //         delete l;
    //     }
    // }

    for (auto B : RPO(this))
    {
        B->print();
    }
}
void ArmBlock::print()
{
    std::cout << name << ":" << std::endl;
    for (auto instr : instrs)
        instr->print();
}
static inline void print_pop_push(ArmInstr *i)
{
    std::cout << " ";
    assert(i->is_arm_push_pop());
    std::cout << "{";
    for (auto op : i->get_ops())
    {
        op->print();
        if (op != i->get_ops().back())
            std::cout << ", ";
    }
    std::cout << "}";
    std::cout << std::endl;
}
static inline void print_movw_gv(ArmInstr *i)
{
    std::cout << " ";
    assert(i->get_ops().size() == 2);
    i->get_ops()[0]->print();
    std::cout << ", " << "#:lower16:";
    i->get_ops()[1]->print();
    std::cout << std::endl;
}
static inline void print_movt_gv(ArmInstr *i)
{
    std::cout << " ";
    assert(i->get_ops().size() == 2);
    i->get_ops()[0]->print();
    std::cout << ", " << "#:upper16:";
    i->get_ops()[1]->print();
    std::cout << std::endl;
}
void ArmInstr::print()
{
    std::cout << "   " << printENUM(armenum);

    if (armenum == ARMENUM::arm_movw && is_a<ArmAddr>(ops[1]))
    {
        print_movw_gv(this);
        return;
    }
    else if (armenum == ARMENUM::arm_movt && is_a<ArmAddr>(ops[1]))
    {
        print_movt_gv(this);
        return;
    }
    else if (armenum == ARMENUM::arm_vmov)
    {
        assert(is_a<ArmReg>(ops[0]));
        if (((ArmReg *)ops[0])->is_s_reg())
            if ((is_a<ArmReg>(ops[1]) && ((ArmReg *)ops[1])->is_s_reg()) || is_a<ArmImmef>(ops[1]))
                std::cout << ".f32";
    }
    else if (is_arm_push_pop())
    {
        print_pop_push(this);
        return;
    }

    std::cout << " ";
    for (auto op : ops)
    {
        op->print();
        if (op != ops.back())
            std::cout << ", ";
    }
    std::cout << std::endl;
    // magic code
    if (armenum == ARMENUM::arm_vcmp_f32)
        std::cout << "   vmrs APSR_nzcv, FPSCR" << std::endl;
}
void ArmAddr::print()
{
    if (is_a<ArmGlobalVariable>(addr))
        std::cout << ((ArmGlobalVariable *)addr)->get_name();
    else if (is_a<ArmBlock>(addr))
        std::cout << ((ArmBlock *)addr)->get_name();
    else if (is_a<ArmFunc>(addr))
        std::cout << ((ArmFunc *)addr)->get_name();
}
void ArmReg::print()
{
    if (is_addr)
        std::cout << "[";
    if (regno == SP)
        std::cout << "sp";
    else if (regno == PC)
        std::cout << "pc";
    else if (regno == LR)
        std::cout << "lr";
    else if (regno <= 15)
        std::cout << "r" << regno;
    else if (regno <= 15 + 32)
        std::cout << "s" << regno - 16;
    else
        assert(0);
    if (is_addr)
    {
        if (offset)
            std::cout << ", " << "#" << offset;
        std::cout << "]";
    }
}
void ArmImme::print()
{
    std::cout << "#" << imme_int;
}
void ArmImmef::print()
{
    std::cout << "#" << imme_float;
}
// deletes
ArmModule::~ArmModule()
{
    for (auto gl : globals)
        delete gl;
    for (auto f : funcs)
        delete f;
}
ArmFunc::~ArmFunc()
{
    for (auto B : blocks)
    {
        for (auto e : *B->get_user_list())
            delete e;
        delete B;
    }
}
ArmBlock::~ArmBlock()
{
    for (auto i : instrs)
    {
        delete i;
    }
}
ArmInstr::~ArmInstr()
{
    for (auto op : ops)
        delete op;
}