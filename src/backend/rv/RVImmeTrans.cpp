#include "backend/rv/RV.hpp"
#include "backend/rv/RVstand.hpp"
#include <backend/rv/RVImmeTrans.hpp>
#include <cassert>
#include <unordered_map>

static inline RVInstr *gen_instr_op2_before(RVENUM e, RVOperand *o1, RVOperand *o2, RVBlock *b, RVInstr *pos = nullptr)
{
    auto newi = new RVInstr(e);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    b->instr_insert_before(pos, newi);
    return newi;
}
static inline RVInstr *gen_instr_op3_before(RVENUM e, RVOperand *o1, RVOperand *o2, RVOperand *o3, RVBlock *b, RVInstr *pos = nullptr)
{
    auto newi = new RVInstr(e);
    newi->ops_push_back(o1);
    newi->ops_push_back(o2);
    newi->ops_push_back(o3);
    b->instr_insert_before(pos, newi);
    return newi;
}
void RVImmeTrans::deal_immef(RVFunc *f)
{
    std::unordered_map<float, RVconstLable *> immefmap;
    for (auto B : f->get_blocks())
    {
        for (auto i : B->get_instrs())
        {
            int flagfonce = 0;
            for (auto op : i->get_ops())
            {
                if (is_a<RVImmef>(op))
                {
                    flagfonce++;
                    RVconstLable *constlable;
                    float immef = ((RVImmef *)op)->get_immef();
                    if (immefmap.find(immef) != immefmap.end())
                    {
                        constlable = immefmap.find(immef)->second;
                    }
                    else
                    {
                        std::string na = ".LC" + f->get_name() + std::to_string(f->get_constlables().size());
                        constlable = new RVconstLable(na, immef);
                        f->constlable_push_backe(constlable);
                    }
                    if (i->get_enum() == RVENUM::rv_l)
                    {
                        gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(t0), new RVAddr(constlable), B, i);
                        i->replace_op_with_and_delete(op, new RVReg_off(t0, 0));
                    }
                    else
                    {
                        gen_instr_op2_before(RVENUM::rv_l, new RVReg_d(t0), new RVAddr(constlable), B, i);
                        gen_instr_op2_before(RVENUM::rv_l, new RVReg(ft0), new RVReg_off(t0, 0), B, i);
                        i->replace_op_with_and_delete(op, new RVReg(ft0));
                    }
                }
            }
            assert(flagfonce <= 1);
        }
    }
}
void RVImmeTrans::deal_large_off(RVFunc *f)
{
    for (auto B : f->get_blocks())
    {
        for (auto i : B->get_instrs())
        {
            for (auto op : i->get_ops())
            {
                if (is_a<RVReg_off>(op))
                    if (!is_legal_i_s_imme_rv(((RVReg_off *)op)->get_off()))
                    {
                        gen_instr_op2_before(RVENUM::rv_l, new RVReg(t1), new RVImme(((RVReg_off *)op)->get_off()), B, i);
                        gen_instr_op3_before(RVENUM::rv_add, new RVReg(t1), new RVReg(sp), new RVReg(t1), B, i);
                        i->replace_op_with_and_delete(op, new RVReg_off(t1, 0));
                    }
            }
        }
    }
}
void RVImmeTrans::run(RVModule *m)
{
    printf("-------Trans------\n");

    for (auto f : m->get_funcs())
    {
        if (f->is_exter())
            continue;
        f->print();
        deal_immef(f);
        deal_large_off(f);
        for (auto B : f->get_blocks())
        {
            for (auto i : B->get_instrs())
            {
                if (i->is_binary())
                {
                    RVOperand *op1 = i->get_op_at(1);
                    RVOperand *op2 = i->get_op_at(2);
                    if (!is_a<RVReg>(op1))
                    {
                        RVReg *newreg = nullptr;
                        if (is_a<RVImme>(op1))
                        {
                            gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), (RVImme *)op1, B, i);
                            newreg = new RVReg(t0);
                            i->replace_op_with(op1, newreg);
                        }
                    }
                    if (is_a<RVImme>(op2) && !is_legal_i_s_imme_rv(((RVImme *)op2)->get_imme()))
                    {
                        gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), op2, B, i);
                        i->replace_op_with(op2, new RVReg(t0));
                    }
                }
                else if (i->get_enum() == RVENUM::rv_s)
                {
                    assert(is_a<RVReg_off>(i->get_op_at(1)));
                    RVOperand *stored = i->get_op_at(0);
                    if (is_a<RVImme>(stored))
                    {
                        if (((RVImme *)stored)->get_imme() == 0)
                        {
                            i->replace_op_with_and_delete(stored, new RVReg(zero));
                        }
                        else
                        {
                            gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), (RVImme *)stored, B, i);
                            i->replace_op_with(stored, new RVReg(t0));
                        }
                    }
                }
                else if (i->get_enum() == RVENUM::rv_b)
                {
                    for (auto op : i->get_ops())
                    {
                        if (is_a<RVImme>(op))
                        {
                            if (((RVImme *)op)->get_imme() == 0)
                            {
                                i->replace_op_with_and_delete(op, new RVReg(zero));
                            }
                            else
                            {
                                gen_instr_op2_before(RVENUM::rv_l, new RVReg(t0), (RVImme *)op, B, i);
                                i->replace_op_with(op, new RVReg(t0));
                            }
                        }
                    }
                }
            }
        }
    }
}