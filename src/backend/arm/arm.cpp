#include <algorithm>
#include <string>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <unistd.h>

#include "../../../include/backend/arm/arm.hpp"

#define ENUM_TO_STRING_CASE(enumeration, value) \
    case enumeration::value:                    \
        print_enum = #value;                    \
        return std::regex_replace(print_enum.substr(4, print_enum.size() - 4), underscore, ".");
std::string printENUM(ARMENUM ARMENUM)
{
    std::string print_enum;
    std::regex underscore("_");
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
void ArmValue::value_list_erase(ArmEdge *e)
{
    auto it = std::find(value_list->begin(), value_list->end(), e);
    assert(it != value_list->end());
    value_list->erase(it);
}
void ArmValue::user_list_erase(ArmEdge *e)
{
    auto it = std::find(user_list->begin(), user_list->end(), e);
    assert(it != user_list->end());
    user_list->erase(it);
}
void ArmEdge::set_user(ArmValue *u)
{
    this->user->value_list_erase(this);

    this->user = u;
    u->value_list_push_back(this);
};
void ArmEdge::drop()
{
    value->user_list_erase(this);
    user->value_list_erase(this);
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
void ArmBlock::instr_insert_before(ArmInstr *pos, ArmInstr *in)
{
    if (pos == nullptr)
    {
        instrs.push_back(in);
        return;
    }

    auto it = std::find(instrs.begin(), instrs.end(), pos);
    assert(it != instrs.end());
    instrs.insert(it, in);
}
void ArmFunc::blocks_erase(ArmBlock *b)
{
    auto it = std::find(blocks.begin(), blocks.end(), b);
    assert(it != blocks.end());
    blocks.erase(it);
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
static inline bool gv_can_bss(std::vector<uint32_t> vs)
{
    for (auto u : vs)
        if ((int)u != 0)
            return false;
    return true;
}

// print
void ArmModule::print(int test)
{
    int saved_stdout = dup(STDOUT_FILENO);
    if (!test)
        freopen(outfile.c_str(), "w", stdout);

    printf(".file \"%s\"\n", infile.c_str());
    printf("   .arch armv7ve\n");
    printf("   .arm\n");
    printf("   .fpu neon-vfpv4\n\n");

    printf("   .section .bss \n");
    std::vector<ArmGlobalVariable *> gvs;
    for (auto g : globals)
    {
        if (gv_can_bss(g->get_words()))
            g->print();
        else
            gvs.push_back(g);
    }
    printf("   .section .data \n");
    for (auto g : gvs)
    {
        g->print();
    }
    printf("\n");
    printf("   .section .text\n");
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
    printf("%s:\n", name.c_str());
    int i = words.size() - 1;
    for (; i >= 0; i--)
        if (words[i] != 0)
            break;

    if (i == -1)
    {
        assert(space);
        printf("   .space %d\n", space);
        return;
    }

    for (int j = 0; j <= i; j++)
    {
        printf("   .word %u\n", words[j]);
    }
    if (space - (i + 1) * 4)
        printf("   .space %d\n", space - (i + 1) * 4);
}
void ArmFunc::print()
{
    if (name == "main")
    {
        printf(".global main\n");
        printf("   .type main, %%function\n");
    }
    printf("%s:\n", name.c_str());
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

    for (auto B : blocks)
    {
        B->print();
    }
    for (auto l : constantlables)
        l->print();
}
void ArmBlock::print()
{
    printf("%s:\n", name.c_str());
    for (auto instr : instrs)
        instr->print();
}
void Armconstlable::print()
{
    printf("%s:\n", name.c_str());
    printf("   .word %u\n", word);
}
static inline void print_pop_push(ArmInstr *i)
{
    printf(" ");
    assert(i->is_arm_push_pop());
    printf("{");
    for (auto op : i->get_ops())
    {
        op->print();
        if (op != i->get_ops().back())
            std::cout << ", ";
    }
    printf("}\n");
}
static inline void print_movw_gv(ArmInstr *i)
{
    printf(" ");
    assert(i->get_ops().size() == 2);
    i->get_ops()[0]->print();
    printf(", #:lower16:");
    i->get_ops()[1]->print();
    printf("\n");
}
static inline void print_movt_gv(ArmInstr *i)
{
    printf(" ");
    assert(i->get_ops().size() == 2);
    i->get_ops()[0]->print();
    printf(", #:upper16:");
    i->get_ops()[1]->print();
    printf("\n");
}
void ArmInstr::print()
{
    printf("   %s", printENUM(armenum).c_str());

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
                printf(".f32");
    }
    else if (is_arm_push_pop())
    {
        print_pop_push(this);
        return;
    }

    printf(" ");
    for (auto op : ops)
    {
        op->print();
        if (op != ops.back())
            printf(", ");
    }
    printf("\n");
    if (armenum == ARMENUM::arm_vcmp_f32)
        printf("   vmrs APSR_nzcv, FPSCR\n");
    fflush(stdout);
}
void ArmAddr::print()
{
    if (is_a<ArmGlobalVariable>(addr))
        printf("%s", ((ArmGlobalVariable *)addr)->get_name().c_str());
    else if (is_a<ArmBlock>(addr))
        printf("%s", ((ArmBlock *)addr)->get_name().c_str());
    else if (is_a<ArmFunc>(addr))
        printf("%s", ((ArmFunc *)addr)->get_name().c_str());
    else if (is_a<Armconstlable>(addr))
        printf("%s", ((Armconstlable *)addr)->get_name().c_str());
}
void ArmReg::print_reg()
{
    if (regno == SP)
        printf("sp");
    else if (regno == PC)
        printf("pc");
    else if (regno == LR)
        printf("lr");
    else if (regno <= 15)
        printf("r%d", regno);
    else if (regno <= 15 + 32)
        printf("s%d", regno - 16);
    else
        assert(0);
}
void ArmReg::print()
{
    if (is_addr)
        printf("[");
    print_reg();
    if (is_addr)
    {
        if (offset)
            printf(", #%d", offset);
        else if (offreg)
        {
            printf(", ");
            offreg->print_reg();
        }
        else
            assert(offset == 0 && offreg == nullptr);
        printf("]");
    }
}
void ArmImme::print()
{
    printf("#%u", imme_int);
}
void ArmImmef::print()
{
    printf("#%f", imme_float);
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
    for (auto l : constantlables)
        delete l;
}
ArmBlock::~ArmBlock()
{
    for (auto i : instrs)
    {
        delete i;
    }
}
ArmReg::~ArmReg()
{
    if (offreg)
    {
        assert(is_addr);
        delete offreg;
    }
}
ArmInstr::~ArmInstr()
{
    for (auto op : ops)
        delete op;
}