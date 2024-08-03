#include "../../../include/backend/arm/BlockMerge.hpp"
#include "../../../include/backend/arm/arm.hpp"

void BlockMerge::single_br(ArmModule *m)
{
    std::vector<ArmBlock *> merge;
    for (auto f : m->get_funcs())
    {
        if (f->is_external())
            continue;
        for (auto BB : RPO(f))
        {
            if (BB->get_instrs().size() == 1)
            {
                if (BB->get_last_instr()->get_enum() == ARMENUM::arm_b)
                {
                    assert(BB->get_user_list()->size() == 1);
                    ArmBlock *succ = (ArmBlock *)BB->get_user_list()->at(0)->user;
                    assert(((ArmAddr *)BB->get_last_instr()->get_op_at(0))->get_addr() == succ);
                    merge.push_back(BB);
                }
            }
        }
    }
    for (auto BB : merge)
    {
        ArmBlock *succ = (ArmBlock *)BB->get_user_list()->at(0)->user;
        // change b addr
        for (auto val : *BB->get_value_list())
        {
            auto pred = (ArmBlock *)val->value;
            for (auto i : pred->get_instrs())
                if (i->is_b())
                {
                    assert(is_a<ArmAddr>(i->get_op_at(0)));
                    if (((ArmAddr *)i->get_op_at(0))->get_addr() == BB)
                    {
                        ((ArmAddr *)i->get_op_at(0))->set_addr(succ);
                    }
                }
        }

        BB->get_user_list()->at(0)->drop();
        auto vsl = *BB->get_value_list();
        for (auto e : vsl)
            e->set_user(succ);

        BB->get_parent()->blocks_erase(BB);
        delete BB;
    }
}
void BlockMerge::run(ArmModule *m)
{
    std::vector<ArmBlock *> merge;
    for (auto f : m->get_funcs())
    {
        if (f->is_external())
            continue;
        for (auto BB : RPO(f))
        {
            if (BB->get_user_list()->size() == 1)
            {
                ArmBlock *succ = (ArmBlock *)BB->get_user_list()->at(0)->user;
                if (succ->get_value_list()->size() == 1)
                {
                    merge.push_back(succ);
                }
            }
        }
    }
    for (auto succ : merge)
    {

        ArmBlock *BB = (ArmBlock *)succ->get_value_list()->at(0)->value;
        BB->print();
        fflush(stdout);

        // change b addr
        for (auto val : *BB->get_value_list())
        {
            auto pred = (ArmBlock *)val->value;
            for (auto i : pred->get_instrs())
                if (i->is_b())
                {
                    assert(is_a<ArmAddr>(i->get_op_at(0)));
                    if (((ArmAddr *)i->get_op_at(0))->get_addr() == BB)
                        ((ArmAddr *)i->get_op_at(0))->set_addr(succ);
                }
        }
        auto instrs = BB->get_instrs();
        ArmInstr *pos = succ->get_instrs().front();
        for (auto in : instrs)
        {
            if (in == instrs.back())
            {
                assert(in->get_enum() == ARMENUM::arm_b);
                assert(is_a<ArmAddr>(in->get_op_at(0)));
                delete in;
            }
            else
            {
                succ->instr_insert_before(pos, in);
            }
        }

        succ->get_value_list()->at(0)->drop();
        auto vsl = *BB->get_value_list();
        for (auto e : vsl)
            e->set_user(succ);

        BB->get_parent()->blocks_erase(BB);
        BB->clear_instrs();
        delete BB;
    }

    single_br(m);
}