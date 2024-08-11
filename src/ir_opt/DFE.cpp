#include "../../include/ir_opt/DFE.hpp"
#include "../../include/ir_opt/SideEffect.hpp"

void DFE::run(Function *f)
{
    std::unordered_map<Function *, bool> sideeffect_func;
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->isCall())
            {
                SideEffect se;
                Function *called = (Function *)i->get_operand_at(0);
                sideeffect_func.emplace(called, se.run(called));
            }
    for (auto BB : *f->get_blocks())
    {
        auto instrs = *BB->get_instrs();
        for (auto i : instrs)
        {
            if (i->isCall())
            {
                if (sideeffect_func.find((Function *)i->get_operand_at(0))->second)
                    continue;

                if (i->get_user_list()->size() == 0)
                    i->drop();
            }
        }
    }
}