#include <iostream>
#include <sstream>

#include "../../include/ir/Value.hpp"
#include "../../include/ir_opt/GVN.hpp"
#include "../../include/util/RPO.hpp"
#include "../../include/ir_opt/ConstFold.hpp"

// TODO :miss
std::unordered_map<InstrutionEnum, std::string> enum_map = {
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

void GVN::run(Function *func)
{
    f = func;
    if (is_debug)
        printf("GVN\n");
    for (auto BB : RPO(f))
    {
        if (is_debug)
        {
            printf("visitBB");
            BB->print_ID();
            printf("\n");
        }
        for (auto instr : *BB->get_instrs())
        {
            visit_instr(instr);
        }
        for (auto phi : *BB->get_phinodes())
        {
            visit_phi(phi);
        }
    }
}

void GVN::visit_instr(Instrution *instr)
{
    Value *deal = nullptr;

    if (instr->isCall())
    { // need call graph???
        return;
    }
    else if (instr->isLoad())
    { // TODO need memery info
        return;
    }
    else if (instr->isStore())
    { // TODO need memery info
        return;
    }
    else if (instr->isBranch() || instr->isJmp() || instr->isAlloca() || instr->isReturn())
    {
        return;
    }
    else if (instr->isCmp() && ((Cmp *)instr)->isCond())
    {
        return;
    }
    else
        deal = const_fold(instr);

    if (deal)
        return;

    Instrution *hit = check_hash(instr);
    if (!hit)
    {
        std::string hashtag = makehashtag(instr);
        hashmap.emplace(hashtag, instr);
        if (is_debug)
            std::cout << hashtag << std::endl;
    }
    else
    {
        if (is_debug)
        {
            printf("hit:");
            hit->print();
            printf("DELETE::");
            instr->print();
        }
        instr->replaceAllUses(hit);
    }
}
void GVN::visit_phi(PHINode *phi)
{
    int first = true;
    Value *eq = nullptr;
    for (auto edge : *phi->get_value_list())
    {
        if (first)
        {
            first = false;
            eq = edge->get_val();
        }
        else if (edge->get_val() == eq)
            continue;
        else
            return;
    }
    printf("visitphi:replacewith");
    // all-phiincome-eq
    assert(eq);
    phi->replaceAllUses(eq);
    eq->print();
}
Instrution *GVN::check_hash(Instrution *instr)
{
    std::string hashtag = makehashtag(instr);
    if (hashmap.find(hashtag) != hashmap.end())
        return hashmap[hashtag];
    else if (canexchangeop(instr))
    {
        std::string hashtag = makereversehashtag(instr);
        if (hashmap.find(hashtag) != hashmap.end())
            return hashmap[hashtag];
    }
    return nullptr;
}
std::string GVN::makehashtag(Instrution *i)
{
    std::stringstream ss;
    for (auto edge : *i->get_value_list())
    {
        Value *val = edge->get_val();
        if (is_a<ConstantI32>(val))
            ss << ((ConstantI32 *)val)->get_i32()[0];
        else if (is_a<ConstantF32>(val))
            ss << ((ConstantF32 *)val)->get_f32()[0];
        else
            ss << static_cast<void *>(val);
    }
    assert(enum_map.find(i->get_Instrtype()) != enum_map.end());
    if (i->isGEP())
    {
        std::string a = ((GEP *)i)->get_isele() ? "0" : "";
        return enum_map[i->get_Instrtype()] + a + ss.str();
    }

    return enum_map[i->get_Instrtype()] + ss.str();
}
std::string GVN::makereversehashtag(Instrution *i)
{
    assert(i->get_value_list()->size() == 2);
    std::stringstream ss;
    auto edges = *i->get_value_list();
    for (auto i = edges.rbegin(); i != edges.rend(); i++)
    {
        Value *val = (*i)->get_val();
        if (is_a<ConstantI32>(val))
            ss << ((ConstantI32 *)val)->get_i32()[0];
        else if (is_a<ConstantF32>(val))
            ss << ((ConstantF32 *)val)->get_f32()[0];
        else
            ss << static_cast<void *>(val);
    }
    return enum_map[i->get_Instrtype()] + ss.str();
}
bool GVN::canexchangeop(Instrution *instr)
{
    if (instr->get_Instrtype() == InstrutionEnum::IADD ||
        instr->get_Instrtype() == InstrutionEnum::FADD)
        return true;
    if (instr->get_Instrtype() == InstrutionEnum::IMUL ||
        instr->get_Instrtype() == InstrutionEnum::FMUL)
        return true;
    // TODO:bit
    return false;
}