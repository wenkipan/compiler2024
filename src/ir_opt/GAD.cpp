#include "../../include/ir_opt/GAD.hpp"

bool GAD::check(Value *GV)
{
    bool flag = false;
    auto users = GV->get_user_list();
    for (auto edge : *users)
    {
        Value *user = edge->get_user();
        assert(is_a<Instrution>(user));
        Instrution *instr = (Instrution *)user;
        switch (instr->get_Instrtype())
        {
        case InstrutionEnum::GEP:
            flag |= check(instr);
        case InstrutionEnum::Store:
            Dels.emplace_back(instr);
            break;
        case InstrutionEnum::Load:
            return true;
            break;
        case InstrutionEnum::Call:
            return true; //todo
            break;
        default:
            assert(0);
        }
    }
    return flag;
}

void GAD::PassRun(Module *p_module)
{
    std::vector<GlobalVariable *> GVs;
    for (GlobalVariable *GV : *p_module->get_globals())
    {
        Dels.clear();
        if (check(GV))
            continue;
        for (Instrution *Del : Dels)
            Del->drop();
        GVs.emplace_back(GV);
    }

    for (GlobalVariable *GV : GVs)
    {
        auto globals = p_module->get_globals();
        auto it = globals->begin();
        for (; it != globals->end(); ++it)
            if ((*it) == GV)
                break;
        assert(it != globals->end());
        globals->erase(it);
        GV->drop();
    }
}