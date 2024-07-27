#include <lir/l_GVN.hpp>
#include <util/RPO.hpp>
#include <iostream>
#include <sstream>
#include <ir_opt/ConstFold.hpp>
// TODO :miss

void l_GVN::run(Function *func)
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

void l_GVN::visit_instr(Instrution *instr)
{

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
void l_GVN::visit_phi(PHINode *phi)
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
Instrution *l_GVN::check_hash(Instrution *instr)
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
std::string l_GVN::makehashtag(Instrution *i)
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
std::string l_GVN::makereversehashtag(Instrution *i)
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
bool l_GVN::canexchangeop(Instrution *instr)
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