#include <backend/arm/BlockMerge.hpp>

void BlockMerge::run(ArmModule *m)
{
    std::vector<ArmBlock *> merge;
    for (auto f : m->get_funcs())
    {
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
        auto instrs = BB->get_instrs();
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
        for (int i = instrs.size() - 2; i >= 0; i--) // b no need
        {
            succ->instrs_insert_before(0, instrs[i]);
        }
        succ->get_value_list()->at(0)->drop();
        for (auto e : *BB->get_value_list())
            succ->get_value_list()->push_back(e);
        delete BB;
    }
}