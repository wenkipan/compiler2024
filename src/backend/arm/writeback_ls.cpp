#include "../../../include/backend/arm/writeback_ls.hpp"

void writeback_ls::run(ArmFunc *f)
{
    for (auto B : f->get_blocks())
    {
        auto instrs = B->get_instrs();
        for (int pos = 0; pos < instrs.size(); pos++)
        {
            ArmInstr *i = instrs[pos];
            if (i->get_enum() == ARMENUM::arm_add && is_a<ArmImme>(i->get_op_at(2)) && is_same_reg((ArmReg *)i->get_op_at(0), (ArmReg *)i->get_op_at(1)))
            {
                ArmReg *dst = (ArmReg *)i->get_op_at(0);
                for (; pos < instrs.size(); pos++)
                {
                    // ArmInstr *cur = instrs[pos];
                    if (instrs[pos]->get_enum() == ARMENUM::arm_str)
                    {
                        assert(is_a<ArmReg>(instrs[pos]->get_op_at(1)));
                        if (is_same_reg(dst, (ArmReg *)instrs[pos]->get_op_at(1)))
                        {
                        }
                    }
                }
            }
        }
    }
}