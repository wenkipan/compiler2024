#pragma once

#include "../../include/ir/ir.hpp"

class GVN_l
{
    std::unordered_map<InstrutionEnum, std::string> enum_map = {
        {InstrutionEnum::Assign, "Assign"},
        {InstrutionEnum::GEP, "GEP"},
        {InstrutionEnum::MINUS, "neg"},
        {InstrutionEnum::F2I, "fptosi"},
        {InstrutionEnum::I2F, "sitofp"},
        {InstrutionEnum::IADD, "add nsw"},
        {InstrutionEnum::ISUB, "sub nsw"},
        {InstrutionEnum::IMUL, "mul nsw"},
        {InstrutionEnum::IDIV, "sdiv"},
        {InstrutionEnum::IMOD, "srem"},
        {InstrutionEnum::FADD, "fadd"},
        {InstrutionEnum::FSUB, "fsub"},
        {InstrutionEnum::FMUL, "fmul"},
        {InstrutionEnum::FDIV, "fdiv"},
        {InstrutionEnum::PADD, "add ptr"},
        {InstrutionEnum::SHL, "<<"},
        {InstrutionEnum::LSHR, ">>"},
        {InstrutionEnum::ASHR, "(Arithmetic)>>"},
        {InstrutionEnum::AND, "&"},
        {InstrutionEnum::OR, "|"},
        {InstrutionEnum::XOR, "^"},
        {InstrutionEnum::IEQ, "icmp eq"},
        {InstrutionEnum::INEQ, "icmp ne"},
        {InstrutionEnum::ILT, "icmp slt"},
        {InstrutionEnum::ILE, "icmp sle"},
        {InstrutionEnum::IGT, "icmp sgt"},
        {InstrutionEnum::IGE, "icmp sge"},
        {InstrutionEnum::FEQ, "fcmp oeq"},
        {InstrutionEnum::FNEQ, "fcmp une"},
        {InstrutionEnum::FLT, "fcmp olt"},
        {InstrutionEnum::FLE, "fcmp ole"},
        {InstrutionEnum::FGT, "fcmp ogt"},
        {InstrutionEnum::FGE, "fcmp oge"},
        {InstrutionEnum::AddSP, "SP +"}};

public:
    int is_debug = 0;
    Function *f;
    std::unordered_map<std::string, Instrution *> hashmap;
    // void init();
    void run(Function *func);
    Instrution *check_hash(Instrution *instr);
    std::string makehashtag(Instrution *i);
    std::string makereversehashtag(Instrution *i);
    bool canexchangeop(Instrution *instr);
    void visit_instr(Instrution *instr);
    void visit_phi(PHINode *phi);
};