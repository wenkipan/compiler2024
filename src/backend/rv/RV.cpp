#include "backend/rv/RVstand.hpp"
#include <backend/rv/RV.hpp>
#include <algorithm>
#include <cassert>
#include <stdio.h>
#include <unistd.h>
// RVInstr::RVInstr(RVENUM i, RVcond c, RVfloat f)
//     : instr_enum(i), cond_enum(c)
// {
//     float_enums.push_back(f);
// }
// RVInstr::RVInstr(RVENUM i, RVcond c, RVfloat f1, RVfloat f2)
//     : instr_enum(i), cond_enum(c)
// {
//     float_enums.push_back(f1);
//     float_enums.push_back(f2);
// }

void RVBlock::instr_insert_before(RVInstr *pos, RVInstr *in)
{
    if (pos == nullptr)
    {
        in->print();
        instrs.push_back(in);
        return;
    }

    auto it = std::find(instrs.begin(), instrs.end(), pos);
    assert(it != instrs.end());
    instrs.insert(it, in);
    in->print();
}
void RVBlock::instr_insert_after(RVInstr *pos, RVInstr *in)
{
    if (pos == nullptr)
    {
        instrs.insert(instrs.begin(), in);
        return;
    }

    auto it = std::find(instrs.begin(), instrs.end(), pos);
    assert(it != instrs.end());
    instrs.insert(it + 1, in);
}
RVEdge::RVEdge(RVValue *v, RVValue *u)
    : val(v), user(u)
{
    val->value_list_push_back(this);
    u->user_list_push_back(this);
}

void RVInstr::replace_op_with(RVOperand *o, RVOperand *newo)
{
    assert(std::find(ops.begin(), ops.end(), o) != ops.end());
    auto it = std::find(ops.begin(), ops.end(), o);
    *it = newo;
}
void RVInstr::replace_op_with_and_delete(RVOperand *o, RVOperand *newo)
{
    assert(std::find(ops.begin(), ops.end(), o) != ops.end());
    auto it = std::find(ops.begin(), ops.end(), o);
    *it = newo;
    delete o;
    this->print();
}

void RVModule::print(int test)
{
    int saved_stdout = dup(STDOUT_FILENO);
    if (!test)
        freopen(outfile.c_str(), "w", stdout);
    printf(".file \"%s\"\n", infile.c_str());
    // printf(".option  march rv32imfd\n");
    // printf(".option   abi ilp32d\n");
    // printf(".attribute stack_align, 16");

    printf("   .section .data \n");
    for (auto g : globals)
    {
        g->print();
    }
    printf("\n");
    printf("   .section .text\n");
    for (auto f : funcs)
    {
        if (f->is_exter())
            continue;
        f->print();
    }

    fflush(stdout);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);
}
void RVGlobalVariable::print()
{
    printf("%s:\n", get_name().c_str());
    int i = words.size() - 1;
    for (; i >= 0; i--)
        if (words[i] != 0)
            break;

    if (i == -1)
    {
        assert(space);
        printf("   .zero %d\n", space);
        return;
    }

    for (int j = 0; j <= i; j++)
    {
        printf("   .word %u\n", words[j]);
    }
    if (space - (i + 1) * 4)
        printf("   .zero %d\n", space - (i + 1) * 4);
}
void RVFunc::print()
{
    if (get_name() == "main")
    {
        printf(".global main\n");
        printf("   .type main, %%function\n");
    }
    for (auto l : constlables)
        l->print();
    printf("%s:\n", get_name().c_str());
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
}
void RVconstLable::print()
{
    printf("%s:\n", get_name().c_str());
    // unsigned long long *ptr = (unsigned long long *)&(word);
    // printf("   .quad 0x%llX\n", *ptr);
    printf("   .word %u\n", *(uint32_t *)(&word));
}
void RVBlock::print()
{
    printf("%s:\n", get_name().c_str());
    for (auto instr : instrs)
        instr->print();
}
void RVInstr::print_enum()
{
    int fflag = 0;
    for (auto op : ops)
        if (is_a<RVReg>(op) && rv_is_f_reg(((RVReg *)op)->get_no()))
        {
            fflag = 1;
            printf("f");
            break;
        }

    switch (instr_enum)
    {
    case RVENUM::rv_add:
        printf("add");
        break;
    case RVENUM::rv_sub:
        printf("sub");
        break;
    case RVENUM::rv_mul:
        printf("mul");
        break;
    case RVENUM::rv_div:
        printf("div");
        break;
    case RVENUM::rv_rem:
        printf("rem");
        break;
    case RVENUM::rv_and:
        printf("and");
        break;
    case RVENUM::rv_or:
        printf("or");
        break;
    case RVENUM::rv_xor:
        printf("xor");
        break;
    case RVENUM::rv_sll:
        printf("sll");
        break;
    case RVENUM::rv_srl:
        printf("srl");
        break;
    case RVENUM::rv_sra:
        printf("sra");
        break;
    case RVENUM::rv_slt:
        printf("slt");
        break;
    case RVENUM::rv_sgt:
        printf("sgt");
        break;
    case RVENUM::rv_sltu:
        printf("sltu");
        break;
    case RVENUM::rv_fadd:
        printf("fadd");
        break;
    case RVENUM::rv_fsub:
        printf("fsub");
        break;
    case RVENUM::rv_fmul:
        printf("fmul");
        break;
    case RVENUM::rv_fdiv:
        printf("fdiv");
        break;
    case RVENUM::rv_fmin:
        printf("fmin");
        break;
    case RVENUM::rv_fmax:
        printf("fmax");
        break;
    case RVENUM::rv_fsqrt:
        printf("fsqrt");
        break;
    case RVENUM::rv_seqz:
        printf("seqz");
        break;
    case RVENUM::rv_snez:
        printf("snez");
        break;
    case RVENUM::rv_mv:
        printf("mv");
        break;
    case RVENUM::rv_neg:
        printf("neg");
        break;
    case RVENUM::rv_b:
        printf("b");
        break;
    case RVENUM::rv_j:
        printf("j");
        break;
    case RVENUM::rv_ret:
        printf("ret");
        break;
    case RVENUM::rv_fcmp:
        printf("");
        break;
    case RVENUM::rv_l:
        printf("l");
        break;
    case RVENUM::rv_s:
        printf("s");
        break;
    case RVENUM::rv_call:
        printf("call");
        break;
    case RVENUM::rv_fcvt_w_s:
        printf("cvt.w.s");
        break;
    case RVENUM::rv_fcvt_s_w:
        printf("cvt.s.w");
        break;
    default:
        printf("Unknown enum value");
        break;
    }
    if (cond_enum != RVcond::nill)
    {
        switch (cond_enum)
        {
        case RVcond::eqz:
            printf("eqz");
            break;
        case RVcond::eq:
            printf("eq");
            break;
        case RVcond::ne:
            printf("ne");
            break;
        case RVcond::lt:
            printf("lt");
            break;
        case RVcond::le:
            printf("le");
            break;
        case RVcond::ge:
            printf("ge");
            break;
        case RVcond::gt:
            printf("gt");
            break;
        case RVcond::ltu:
            printf("ltu");
            break;
        case RVcond::geu:
            printf("geu");
            break;
        default:
            printf("Unknown enum value");
            break;
        }
    }

    if (instr_enum == RVENUM::rv_l)
    {
        if (is_a<RVReg_d>(ops[0]) && is_a<RVAddr>(ops[1]))
            printf("a");
        else if (is_a<RVImme>(ops[1]))
            printf("i");
        else if (is_a<RVReg_d>(ops[0]))
            printf("d");
        else
            printf("w");
    }
    if (instr_enum == RVENUM::rv_s)
    {
        if (is_a<RVReg_d>(ops[0]))
            printf("d");
        else if (is_a<RVReg>(ops[0]))
            printf("w");
    }
    if ((instr_enum == RVENUM::rv_add || instr_enum == RVENUM::rv_sub) && is_a<RVImme>(ops[2]))
        printf("i");
    else if (instr_enum == RVENUM::rv_add || instr_enum == RVENUM::rv_sub || instr_enum == RVENUM::rv_mul ||
             instr_enum == RVENUM::rv_div || instr_enum == RVENUM::rv_rem)
    {
        if (is_a<RVReg>(ops[1]) && !is_a<RVReg_d>(ops[1]) && ((RVReg *)ops[1])->is_r_reg() &&
            is_a<RVReg>(ops[2]) && !is_a<RVReg_d>(ops[2]) && ((RVReg *)ops[2])->is_r_reg())
        {
            printf("w");
        }
    }
    // begin f lags
    if (fflag)
    {
        if (instr_enum == RVENUM::rv_fcvt_s_w || instr_enum == RVENUM::rv_fcvt_w_s ||
            instr_enum == RVENUM::rv_l || instr_enum == RVENUM::rv_s)
        {
        }
        else
            printf(".s");
    }
}
void RVInstr::print()
{
    printf("   ");
    print_enum();
    printf(" ");
    for (auto op : ops)
    {
        op->print();
        if (op != ops.back())
            printf(", ");
    }
    if (instr_enum == RVENUM::rv_fcvt_w_s)
        printf(",rtz");
    printf("\n");
}
std::unordered_map<int, std::string> rv_registers = {
    {0, "zero"}, {1, "ra"}, {2, "sp"}, {3, "gp"}, {4, "tp"}, {5, "t0"}, {6, "t1"}, {7, "t2"}, {8, "s0"}, {9, "s1"}, {10, "a0"}, {11, "a1"}, {12, "a2"}, {13, "a3"}, {14, "a4"}, {15, "a5"}, {16, "a6"}, {17, "a7"}, {18, "s2"}, {19, "s3"}, {20, "s4"}, {21, "s5"}, {22, "s6"}, {23, "s7"}, {24, "s8"}, {25, "s9"}, {26, "s10"}, {27, "s11"}, {28, "t3"}, {29, "t4"}, {30, "t5"}, {31, "t6"}, {32, "ft0"}, {33, "ft1"}, {34, "ft2"}, {35, "ft3"}, {36, "ft4"}, {37, "ft5"}, {38, "ft6"}, {39, "ft7"}, {40, "fs0"}, {41, "fs1"}, {42, "fa0"}, {43, "fa1"}, {44, "fa2"}, {45, "fa3"}, {46, "fa4"}, {47, "fa5"}, {48, "fa6"}, {49, "fa7"}, {50, "fs2"}, {51, "fs3"}, {52, "fs4"}, {53, "fs5"}, {54, "fs6"}, {55, "fs7"}, {56, "fs8"}, {57, "fs9"}, {58, "fs10"}, {59, "fs11"}, {60, "ft8"}, {61, "ft9"}, {62, "ft10"}, {63, "ft11"}};
void RVReg::print()
{
    assert(rv_registers.find(rno) != rv_registers.end());
    std::string rna = rv_registers.find(rno)->second;
    printf("%s", rna.c_str());
}
void RVAddr::print()
{
    if (is_a<RVGlobalVariable>(label))
        printf("%s", ((RVGlobalVariable *)label)->get_name().c_str());
    else if (is_a<RVBlock>(label))
        printf("%s", ((RVBlock *)label)->get_name().c_str());
    else if (is_a<RVFunc>(label))
        printf("%s", ((RVFunc *)label)->get_name().c_str());
    else if (is_a<RVconstLable>(label))
        printf("%s", ((RVconstLable *)label)->get_name().c_str());
}
void RVReg_off::print()
{
    printf("%d", offset);
    printf("(");
    assert(rv_registers.find(rno) != rv_registers.end());
    std::string rna = rv_registers.find(rno)->second;
    printf("%s", rna.c_str());
    printf(")");
}
void RVImme::print()
{
    printf("%d", imme);
}
void RVImmef::print()
{
    printf("%f", imme);
}

RVModule::~RVModule()
{
    for (auto gl : globals)
        delete gl;
    for (auto f : funcs)
        delete f;
}
RVFunc::~RVFunc()
{
    for (auto B : blocks)
    {
        for (auto e : B->get_user_list())
            delete e;
        delete B;
    }
    for (auto l : constlables)
        delete l;
}
RVBlock::~RVBlock()
{
    for (auto i : instrs)
    {
        delete i;
    }
}
RVInstr::~RVInstr()
{
    for (auto op : ops)
        delete op;
}
RVReg::~RVReg() {}
RVReg_off::~RVReg_off() {}
RVImme::~RVImme() {}
RVImmef::~RVImmef() {}