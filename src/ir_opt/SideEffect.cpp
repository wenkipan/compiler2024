#include "../../include/ir_opt/SideEffect.hpp"

static inline Value *trace_back_to(Instrution *i)
{
    assert(i->isStore() || i->isGEP() || i->isLoad());
    Value *addr = i->get_operand_at(0);
    if (is_a<GEP>(addr))
        return trace_back_to((Instrution *)addr);

    return addr;
}
bool SideEffect::check_mem(Function *f)
{
    for (auto BB : *f->get_blocks())
        for (auto i : *BB->get_instrs())
            if (i->isStore())
            {
                Value *src = trace_back_to(i);
                if (is_a<GlobalVariable>(src))
                    return true;
                else if (is_a<Param>(src))
                    return true;
            }
            else if (i->isCall())
            {
                Function *called = (Function *)i->get_operand_at(0);
                if (called->get_isExternal())
                    return true;
                else if (called != f)
                {
                    SideEffect check;
                    if (check.run(called))
                        return true;
                }
            }
            else if (i->get_Instype() == InstrutionEnum::Assign)
            {
                if (is_a<GlobalVariable>(i->get_operand_at(0)))
                    return true;
            }
    return false;
}
bool SideEffect::run(Function *f)
{
    if (f->get_isExternal())
        return true;
    if (check_mem(f))
        return true;
    return false;
}