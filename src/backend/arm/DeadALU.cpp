#include "../../../include/backend/arm/DeadALU.hpp"

void DeadALU::run(ArmFunc *f)
{
    for (auto B : f->get_blocks())
    {
        for (auto i : B->get_instrs())
        {
            if (i->get_enum() == ARMENUM::arm_add &&
                is_a<ArmImme>(i->get_op_at(2)) &&
                is_same_reg((ArmReg *)i->get_op_at(0), (ArmReg *)i->get_op_at(1)))
            {
                if (((ArmImme *)i->get_op_at(2))->get_imme() == (uint32_t)0)
                {
                    B->erase_instr(i);
                    delete i;
                }
            }
        }
    }
}